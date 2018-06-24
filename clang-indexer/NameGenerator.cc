#include "NameGenerator.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclVisitor.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/raw_ostream.h>
#include <string>

#include "Switcher.h"
#include "Util.h"

namespace indexer {


///////////////////////////////////////////////////////////////////////////////
// NameGenerator class definition

class NameGenerator : public clang::DeclVisitor<NameGenerator>
{
public:
    NameGenerator(clang::NamedDecl *originalDecl, std::string &output);
    bool error() { return m_error; }
    void VisitDeclContext(clang::DeclContext *context);
    void VisitNamespaceDecl(clang::NamespaceDecl *decl);
    void VisitClassTemplateSpecializationDecl(
            clang::ClassTemplateSpecializationDecl *decl);
    void VisitTagDecl(clang::TagDecl *decl);
    void VisitVarDecl(clang::VarDecl *decl);
    void VisitFunctionDecl(clang::FunctionDecl *decl);
    void VisitNamedDecl(clang::NamedDecl *decl);

private:
    void outputSeparator();
    void outputFunctionIdentifier(clang::DeclarationName name);

private:
    clang::NamedDecl *m_originalDecl;
    bool m_error;
    bool m_needSeparator;
    bool m_needFilePrefix;
    bool m_needOffsetPrefix;
    bool m_inDeclContext;
    llvm::raw_string_ostream m_out;
};


///////////////////////////////////////////////////////////////////////////////
// NameGenerator implementation

NameGenerator::NameGenerator(clang::NamedDecl *originalDecl, std::string &output) :
    m_originalDecl(originalDecl),
    m_error(false),
    m_needSeparator(false),
    m_needFilePrefix(false),
    m_needOffsetPrefix(false),
    m_inDeclContext(false),
    m_out(output)
{
}

void NameGenerator::outputSeparator()
{
    if (!m_needSeparator)
        return;
    m_out << "::";
    m_needSeparator = false;
}

void NameGenerator::VisitDeclContext(clang::DeclContext *context)
{
    if (clang::NamedDecl *decl =
            llvm::dyn_cast_or_null<clang::NamedDecl>(context)) {
        if (clang::FunctionDecl *funcDecl =
                llvm::dyn_cast<clang::FunctionDecl>(decl)) {
            if (funcDecl->isThisDeclarationADefinition()) {
                // For declarations inside a function body, prefix both a
                // filename and a file offset.
                m_needFilePrefix = true;
                m_needOffsetPrefix = true;
            }
        }
        Switcher<bool> sw1(m_inDeclContext, true);
        Visit(decl);
        return;
    }

    if (!m_needFilePrefix)
        return;

    // The symbol has internal linkage, so prepend a filename to the symbol
    // name to help disambiguate it from symbols in other files with the same
    // name.  It's common to define something with internal linkage in a header
    // file that's shared between many translation units in a program (e.g.
    // static functions, constant variables), so use the name of the file
    // containing the declaration rather than the name of the translation unit.
    // Use just the basename to keep the symbol name short.

    clang::Decl *decl = m_originalDecl;
    if (clang::Decl *canonicalDecl = decl->getCanonicalDecl())
        decl = canonicalDecl;
    clang::SourceManager &sourceManager =
            decl->getASTContext().getSourceManager();
    clang::SourceLocation sloc = sourceManager.getSpellingLoc(
                decl->getLocation());
    if (!sloc.isInvalid()) {
        clang::FileID fileID = sourceManager.getFileID(sloc);
        if (!fileID.isInvalid()) {
            const clang::FileEntry *fileEntry =
                    sourceManager.getFileEntryForID(fileID);
            if (fileEntry != NULL) {
                llvm::StringRef fname = fileEntry->getName();
                m_out << const_basename(fname.data());
                if (m_needOffsetPrefix)
                    m_out << '@' << sourceManager.getFileOffset(sloc);
                m_out << '/';
            }
        }
    }
}

void NameGenerator::VisitNamespaceDecl(clang::NamespaceDecl *decl)
{
    if (decl->isAnonymousNamespace()) {
        m_needFilePrefix = true;
        VisitDeclContext(decl->getDeclContext());
        outputSeparator();
        m_out << "<anon>";
        m_needSeparator = true;
    } else {
        VisitDeclContext(decl->getDeclContext());
        outputSeparator();
        decl->printName(m_out);
        m_needSeparator = true;
    }
}

void NameGenerator::VisitClassTemplateSpecializationDecl(
        clang::ClassTemplateSpecializationDecl *decl)
{
    VisitTagDecl(decl);

    // Only add template arguments for explicit specializations, not for
    // instantiations.  This loop will skip over implicit instantiations of
    // partial specializations.  (e.g. It will turn
    // std::vector<bool, std::allocator<bool>> into
    // std::vector<bool, type-parameter-0-0>).
    while (decl->getSpecializationKind() != clang::TSK_ExplicitSpecialization) {
        llvm::PointerUnion<
                clang::ClassTemplateDecl*,
                clang::ClassTemplatePartialSpecializationDecl*>
            instantiatedFrom = decl->getInstantiatedFrom();
        clang::ClassTemplatePartialSpecializationDecl *partialSpec =
                instantiatedFrom.dyn_cast<
                        clang::ClassTemplatePartialSpecializationDecl*>();
        if (partialSpec != NULL) {
            decl = partialSpec;
            continue;
        }
        return;
    }

    m_out << '<';
    const clang::TemplateArgumentList &args = decl->getTemplateArgs();
    for (unsigned i = 0, iEnd = args.size(); i != iEnd; ++i) {
        if (i != 0)
            m_out << ", ";
        args[i].print(decl->getASTContext().getPrintingPolicy(), m_out);
    }
    m_out << '>';
}

void NameGenerator::VisitTagDecl(clang::TagDecl *decl)
{
    VisitDeclContext(decl->getDeclContext());

    // Leave the enum name out of the symbol names of the enumerators.
    if (m_inDeclContext && decl->isEnum())
        return;

    // For "typedef struct { ... } S", refer to the struct using S.
    clang::NamedDecl *namedDecl = decl;
    clang::TypedefNameDecl *anonDecl = decl->getTypedefNameForAnonDecl();
    if (anonDecl != NULL)
        namedDecl = anonDecl;

    outputSeparator();
    clang::IdentifierInfo *identifier = namedDecl->getIdentifier();
    if (identifier == NULL) {
        m_out << "<unnamed" <<
                 decl->getASTContext().getSourceManager().getFileOffset(
                     decl->getLocation()) << '>';
    } else {
        m_out << identifier->getName();
    }
    m_needSeparator = true;
}

void NameGenerator::VisitVarDecl(clang::VarDecl *decl)
{
    if (!decl->isExternC() && !decl->hasExternalStorage())
        m_needFilePrefix = true;
    VisitDeclContext(decl->getDeclContext());

    outputSeparator();
    decl->printName(m_out);
    m_needSeparator = true;
}

// Write the name of the function to the output.  Do not write any qualifiers
// or template parameters/arguments (i.e. no colons and no brackets).
void NameGenerator::outputFunctionIdentifier(clang::DeclarationName name)
{
    // DeclarationName::printName includes template parameters for the
    // constructors and destructors of template classes.  Omit these by digging
    // into the name for the InjectedClassName type.
    clang::DeclarationName::NameKind nameKind = name.getNameKind();
    if (nameKind == clang::DeclarationName::CXXConstructorName ||
            nameKind == clang::DeclarationName::CXXDestructorName) {
        const clang::Type *nameType = name.getCXXNameType().getTypePtr();
        assert(nameType != NULL);
        const clang::InjectedClassNameType *injectedNameType =
                nameType->getAs<clang::InjectedClassNameType>();
        if (injectedNameType != NULL) {
            if (nameKind == clang::DeclarationName::CXXDestructorName)
                m_out << '~';
            m_out << injectedNameType->getDecl()->getName();
            return;
        }
    }

    m_out << name.getAsString();
}

void NameGenerator::VisitFunctionDecl(clang::FunctionDecl *decl)
{
    clang::PrintingPolicy policy = decl->getASTContext().getPrintingPolicy();
    policy.SuppressTagKeyword = true;
    bool isExternC = false;

    // When a function is only marked extern "C" in a header file but not in
    // the source file, there are two declarations, where only one
    // declaration's isExternC is true.  Use the isExternC flag on the
    // canonical declaration for consistency.
    if (clang::FunctionDecl *canonical = decl->getCanonicalDecl())
        isExternC = canonical->isExternC();

    if (decl->getTemplateInstantiationPattern() != NULL)
        decl = decl->getTemplateInstantiationPattern();

    // TODO: Review correctness for C code.
    if (!isExternC && !decl->isInExternCContext()
          && !decl->isInExternCXXContext())
        m_needFilePrefix = true;

    VisitDeclContext(decl->getDeclContext());
    outputSeparator();
    outputFunctionIdentifier(decl->getDeclName());

    // Print the template arguments for template functions (not for
    // non-template methods of template classes), and for explicit
    // specializations only -- not for an instantiations of a template pattern.
    if (const clang::TemplateArgumentList *templateArgs =
            decl->getTemplateSpecializationArgs()) {
        m_out << '<';
        for (unsigned int i = 0; i < templateArgs->size(); ++i) {
            if (i != 0)
                m_out << ", ";
            templateArgs->get(i).print(policy, m_out);
        }
        m_out << '>';
    }

    // For extern "C++" function declarations, include the parameter types in
    // the name.
    if (decl->getASTContext().getLangOpts().CPlusPlus && !isExternC) {
        m_out << '(';
        bool firstParm = true;
        for (auto it = decl->param_begin(), itEnd = decl->param_end();
                it != itEnd; ++it) {
            clang::ParmVarDecl *parm = *it;
            if (!firstParm)
                m_out << ", ";
            clang::QualType parmType = parm->getType();
            parmType = parmType->getCanonicalTypeUnqualified();
            m_out << parmType.getAsString(policy);
            firstParm = false;
        }
        m_out << ')';

        // C++ method type qualifiers.
        if (clang::CXXMethodDecl *method =
                llvm::dyn_cast<clang::CXXMethodDecl>(decl)) {
            unsigned quals = method->getTypeQualifiers();
            if (quals & clang::Qualifiers::Const)
                m_out << " const";
            if (quals & clang::Qualifiers::Restrict)
                m_out << " restrict";
            if (quals & clang::Qualifiers::Volatile)
                m_out << " volatile";
        }
    }
    m_needSeparator = true;
}

// Handle the remaining kinds of named decls that weren't handled above.
void NameGenerator::VisitNamedDecl(clang::NamedDecl *decl)
{
    VisitDeclContext(decl->getDeclContext());
    outputSeparator();
    decl->printName(m_out);
    m_needSeparator = true;
}


///////////////////////////////////////////////////////////////////////////////
// getDeclName

bool getDeclName(clang::NamedDecl *decl, std::string &output)
{
    NameGenerator generator(decl, output);
    generator.Visit(decl);
    return !generator.error();
}

} // namespace indexer
