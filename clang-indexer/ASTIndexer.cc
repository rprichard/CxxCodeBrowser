#include "ASTIndexer.h"

#include <clang/AST/Type.h>
#include <clang/Lex/Preprocessor.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/Casting.h>
#include <string>

#include "IndexBuilder.h"
#include "IndexerContext.h"
#include "NameGenerator.h"
#include "Switcher.h"

namespace indexer {

// TODO: For an array access, X[I], skip over the array-to-pointer decay.  We
// currently record that X's address is taken, which is technically true, but
// the address does not generally escape.

// TODO: A struct assignment in C++ turns into an operator= call, even if the
// user did not define an operator= function.  Consider handling operator=
// calls the same way that normal assignment operations are handled.  Consider
// doing the same for the other overloadable operators.

// TODO: Unary ++ and --
// TODO: Fix local extern variables.  e.g. void func() { extern int var; }
//     Consider extern "C", functions nested within classes or namespaces.

///////////////////////////////////////////////////////////////////////////////
// Misc routines

ASTIndexer::ASTIndexer(IndexerContext &indexerContext) :
    m_indexerContext(indexerContext),
    m_thisContext(0),
    m_childContext(0),
    m_typeContext(RT_Reference)
{
}

///////////////////////////////////////////////////////////////////////////////
// Dispatcher routines

// Set the "this" context to the "child" context and reset the child context
// to default.

bool ASTIndexer::TraverseStmt(clang::Stmt *stmt)
{
    if (stmt == NULL)
        return true;

    Switcher<Context> sw1(m_thisContext, m_childContext);

    if (clang::Expr *e = llvm::dyn_cast<clang::Expr>(stmt)) {
        if (e->isRValue())
            m_thisContext &= ~(CF_AddressTaken | CF_Assigned | CF_Modified);
    } else {
        m_thisContext = 0;
    }

    Switcher<Context> sw2(m_childContext, m_thisContext);

    return base::TraverseStmt(stmt);
}

bool ASTIndexer::TraverseType(clang::QualType t)
{
    Switcher<Context> sw1(m_thisContext, 0);
    Switcher<Context> sw2(m_childContext, 0);
    return base::TraverseType(t);
}

bool ASTIndexer::TraverseTypeLoc(clang::TypeLoc tl)
{
    Switcher<Context> sw1(m_thisContext, 0);
    Switcher<Context> sw2(m_childContext, 0);
    return base::TraverseTypeLoc(tl);
}

bool ASTIndexer::TraverseDecl(clang::Decl *d)
{
    Switcher<Context> sw1(m_thisContext, 0);
    Switcher<Context> sw2(m_childContext, 0);
    return base::TraverseDecl(d);
}


///////////////////////////////////////////////////////////////////////////////
// Expression context propagation

bool ASTIndexer::TraverseCallCommon(clang::CallExpr *call)
{
    {
        m_childContext = CF_Called;
        TraverseStmt(call->getCallee());
    }
    for (unsigned int i = 0; i < call->getNumArgs(); ++i) {
        clang::Expr *arg = call->getArg(i);
        // If the child is really an lvalue, then this call is passing a
        // reference.  If the child is not an lvalue, then this address-taken
        // flag will be masked out, and the rvalue will be assumed to be read.
        m_childContext = CF_AddressTaken;
        TraverseStmt(arg);
    }
    return true;
}

bool ASTIndexer::TraverseBinComma(clang::BinaryOperator *s)
{
    {
        m_childContext = 0;
        TraverseStmt(s->getLHS());
    }
    {
        m_childContext = m_thisContext;
        TraverseStmt(s->getRHS());
    }
    return true;
}

bool ASTIndexer::TraverseAssignCommon(
        clang::BinaryOperator *e,
        ContextFlags lhsFlag)
{
    {
        m_childContext = m_thisContext | lhsFlag;
        if (m_childContext & CF_Called) {
            m_childContext ^= CF_Called;
            m_childContext |= CF_Read;
        }
        TraverseStmt(e->getLHS());
    }
    {
        m_childContext = 0;
        TraverseStmt(e->getRHS());
    }
    return true;
}

bool ASTIndexer::VisitCastExpr(clang::CastExpr *e)
{
    if (e->getCastKind() == clang::CK_ArrayToPointerDecay) {
        // Note that e->getSubExpr() can be an rvalue array, in which case the
        // CF_AddressTaken context will be masked away to 0.
        m_childContext = CF_AddressTaken;
    } else if (e->getCastKind() == clang::CK_ToVoid) {
        m_childContext = 0;
    } else if (e->getCastKind() == clang::CK_LValueToRValue) {
        m_childContext = CF_Read;
    }
    return true;
}

bool ASTIndexer::VisitUnaryAddrOf(clang::UnaryOperator *e)
{
    m_childContext = CF_AddressTaken;
    return true;
}

bool ASTIndexer::VisitUnaryDeref(clang::UnaryOperator *e)
{
    m_childContext = 0;
    return true;
}

bool ASTIndexer::VisitDeclStmt(clang::DeclStmt *s)
{
    // If a declaration is a reference to an lvalue initializer, then we
    // record that that the initializer's address was taken.  If it is
    // an rvalue instead, then we'll see something else in the tree indicating
    // what kind of reference to record (e.g. lval-to-rval cast, assignment,
    // call, & operator) or assume the rvalue is being read.
    m_childContext = CF_AddressTaken;
    return true;
}

bool ASTIndexer::VisitReturnStmt(clang::ReturnStmt *s)
{
    // See comment for VisitDeclStmt.
    m_childContext = CF_AddressTaken;
    return true;
}

bool ASTIndexer::VisitVarDecl(clang::VarDecl *d)
{
    // See comment for VisitDeclStmt.  Set the context for the variable's
    // initializer.  Also handle default arguments on parameter declarations.
    m_childContext = CF_AddressTaken;
    return true;
}

bool ASTIndexer::VisitInitListExpr(clang::InitListExpr *e)
{
    // See comment for VisitDeclStmt.  An initializer list can also bind
    // references.
    m_childContext = CF_AddressTaken;
    return true;
}

bool ASTIndexer::TraverseConstructorInitializer(clang::CXXCtorInitializer *init)
{
    if (init->getMember() != NULL) {
        RecordDeclRef(init->getMember(),
                      init->getMemberLocation(),
                      RT_Initialized);
    }

    // See comment for VisitDeclStmt.
    m_childContext = CF_AddressTaken;
    return base::TraverseConstructorInitializer(init);
}


///////////////////////////////////////////////////////////////////////////////
// Expression reference recording

bool ASTIndexer::VisitMemberExpr(clang::MemberExpr *e)
{
    RecordDeclRefExpr(
                e->getMemberDecl(),
                e->getMemberLoc(),
                e,
                m_thisContext);

    // Update the child context for the base sub-expression.
    if (e->isArrow()) {
        m_childContext = CF_Read;
    } else {
        m_childContext = 0;
        if (m_thisContext & CF_AddressTaken)
            m_childContext |= CF_AddressTaken;
        if (m_thisContext & (CF_Assigned | CF_Modified))
            m_childContext |= CF_Modified;
        if (m_thisContext & CF_Read)
            m_childContext |= CF_Read;
        if (m_thisContext & CF_Called) {
            // I'm not sure what the best behavior here is.
            m_childContext |= CF_Read;
        }
    }

    return true;
}

bool ASTIndexer::VisitDeclRefExpr(clang::DeclRefExpr *e)
{
    RecordDeclRefExpr(
                e->getDecl(),
                e->getLocation(),
                e,
                m_thisContext);
    return true;
}

bool ASTIndexer::VisitCXXConstructExpr(clang::CXXConstructExpr *e)
{
    if (e->getParenOrBraceRange().isValid()) {
        // XXX: This code is a kludge.  Recording calls to constructors is
        // troublesome because there isn't an obvious location to associate the
        // call with.  Consider:
        //     A::A() : field(1, 2, 3) {}
        //     new A<B>(1, 2, 3)
        //     struct A { A(B); }; A f() { B b; return b; }
        // Implicit calls to conversion operator methods pose a similar
        // problem.
        //
        // Recording constructor calls is very useful, though, so, as a
        // temporary measure, when there are constructor arguments surrounded
        // by parentheses, associate the call with the right parenthesis.
        //
        // Perhaps the right fix is to associate the call with the line itself
        // or with a larger span which may have other references nested within
        // it.  The fix may have implications for the navigator GUI.
        RecordDeclRefExpr(
                    e->getConstructor(),
                    e->getParenOrBraceRange().getEnd(),
                    e,
                    CF_Called);
    }
    return true;
}

void ASTIndexer::RecordDeclRefExpr(clang::NamedDecl *d, clang::SourceLocation loc, clang::Expr *e, Context context)
{
    if (llvm::isa<clang::FunctionDecl>(*d)) {
        // XXX: This code seems sloppy, but I suspect it will work well enough.
        if (context & CF_Called)
            RecordDeclRef(d, loc, RT_Called);
        if (!(context & CF_Called) || (context & (CF_Read | CF_AddressTaken)))
            RecordDeclRef(d, loc, RT_AddressTaken);
    } else {
        if (context & CF_Called)
            RecordDeclRef(d, loc, RT_Called);
        if (context & CF_Read)
            RecordDeclRef(d, loc, RT_Read);
        if (context & CF_AddressTaken)
            RecordDeclRef(d, loc, RT_AddressTaken);
        if (context & CF_Assigned)
            RecordDeclRef(d, loc, RT_Assigned);
        if (context & CF_Modified)
            RecordDeclRef(d, loc, RT_Modified);
        if (context == 0)
            RecordDeclRef(d, loc, e->isRValue() ? RT_Read : RT_Other);
    }
}


///////////////////////////////////////////////////////////////////////////////
// NestedNameSpecifier handling

bool ASTIndexer::TraverseNestedNameSpecifierLoc(
        clang::NestedNameSpecifierLoc qualifier)
{
    for (; qualifier; qualifier = qualifier.getPrefix()) {
        clang::NestedNameSpecifier *nns = qualifier.getNestedNameSpecifier();
        switch (nns->getKind()) {
        case clang::NestedNameSpecifier::Namespace:
            RecordDeclRef(nns->getAsNamespace(),
                          qualifier.getLocalBeginLoc(),
                          RT_Qualifier);
            break;
        case clang::NestedNameSpecifier::NamespaceAlias:
            RecordDeclRef(nns->getAsNamespaceAlias(),
                          qualifier.getLocalBeginLoc(),
                          RT_Qualifier);
            break;
        case clang::NestedNameSpecifier::TypeSpec:
        case clang::NestedNameSpecifier::TypeSpecWithTemplate:
            if (const clang::TypedefType *tt = nns->getAsType()->getAs<clang::TypedefType>()) {
                RecordDeclRef(tt->getDecl(),
                              qualifier.getLocalBeginLoc(),
                              RT_Qualifier);
            } else if (const clang::RecordType *rt = nns->getAsType()->getAs<clang::RecordType>()) {
                RecordDeclRef(rt->getDecl(),
                              qualifier.getLocalBeginLoc(),
                              RT_Qualifier);
            } else if (const clang::TemplateSpecializationType *tst =
                       nns->getAsType()->getAs<clang::TemplateSpecializationType>()) {

                if (clang::TemplateDecl *decl = tst->getTemplateName().getAsTemplateDecl()) {
                    if (clang::NamedDecl *templatedDecl = decl->getTemplatedDecl()) {
                        RecordDeclRef(templatedDecl,
                                      qualifier.getLocalBeginLoc(),
                                      RT_Qualifier);

                    }
                }
            }
            break;
        default:
            // TODO: Do these cases matter?
            break;
        }
    }
    return true;
}


///////////////////////////////////////////////////////////////////////////////
// Declaration and TypeLoc handling

void ASTIndexer::traverseDeclContextHelper(clang::DeclContext *d)
{
    // Traverse children.
    for (clang::DeclContext::decl_iterator it = d->decls_begin(),
            itEnd = d->decls_end(); it != itEnd; ++it) {
        // BlockDecls are traversed through BlockExprs.
        if (!llvm::isa<clang::BlockDecl>(*it))
            TraverseDecl(*it);
    }
}

// Overriding TraverseCXXRecordDecl lets us mark the base-class references
// with the "Base-Class" kind.
bool ASTIndexer::TraverseCXXRecordDecl(clang::CXXRecordDecl *d)
{
    // Traverse qualifiers on the record decl.
    TraverseNestedNameSpecifierLoc(d->getQualifierLoc());

    // Visit the TagDecl to record its ref.
    WalkUpFromCXXRecordDecl(d);

    // Traverse base classes.
    if (d->isThisDeclarationADefinition()) {
        for (clang::CXXRecordDecl::base_class_iterator it = d->bases_begin();
                it != d->bases_end();
                ++it) {
            clang::CXXBaseSpecifier *baseSpecifier = it;
            Switcher<RefType> sw(m_typeContext, RT_BaseClass);
            TraverseTypeLoc(baseSpecifier->getTypeSourceInfo()->getTypeLoc());
        }
    }

    traverseDeclContextHelper(d);
    return true;
}

bool ASTIndexer::TraverseClassTemplateSpecializationDecl(
        clang::ClassTemplateSpecializationDecl *d)
{
    // base::TraverseClassTemplateSpecializationDecl calls TraverseTypeLoc,
    // which then visits a clang::TemplateSpecializationTypeLoc.  We want
    // to mark the template specialization as Declaration or Definition,
    // not Reference, so skip the TraverseTypeLoc call.
    //
    // The problem happens with code like this:
    //     template <>
    //     struct Vector<bool, void> {};

    WalkUpFromClassTemplateSpecializationDecl(d);
    traverseDeclContextHelper(d);
    return true;
}

bool ASTIndexer::TraverseNamespaceAliasDecl(clang::NamespaceAliasDecl *d)
{
    // The base::TraverseNamespaceAliasDecl function avoids traversing the
    // namespace decl itself because (I think) that would traverse the
    // complete contents of the namespace.  However, it fails to traverse
    // the qualifiers on the target namespace, so we do that here.
    TraverseNestedNameSpecifierLoc(d->getQualifierLoc());

    return base::TraverseNamespaceAliasDecl(d);
}

void ASTIndexer::templateParameterListsHelper(clang::DeclaratorDecl *d)
{
    for (unsigned i = 0, iEnd = d->getNumTemplateParameterLists();
            i != iEnd; ++i) {
        clang::TemplateParameterList *parmList =
                d->getTemplateParameterList(i);
        for (clang::NamedDecl *parm : *parmList)
            TraverseDecl(parm);
    }
}

bool ASTIndexer::VisitDecl(clang::Decl *d)
{
    if (clang::NamedDecl *nd = llvm::dyn_cast<clang::NamedDecl>(d)) {
        clang::SourceLocation loc = nd->getLocation();
        if (clang::FunctionDecl *fd = llvm::dyn_cast<clang::FunctionDecl>(d)) {
            if (fd->getTemplateInstantiationPattern() != NULL) {
                // When Clang instantiates a function template, it seems to
                // create a FunctionDecl for the instantiation that returns
                // false for fd->isThisDeclarationADefinition().  The result
                // is that the template function definition's location is
                // marked as both a Declaration and a Definition.  Fix this by
                // omitting the ref on the instantiation.
            } else {
#if 0
                // This code recorded refs without appropriate qualifiers.  For
                // example, with the code
                //     template <typename A> void Vector<A>::clear() {}
                // it would record the first A as "A", but it needs to record
                // Vector::A.
                templateParameterListsHelper(fd);
#endif
                RefType refType;
                refType = fd->isThisDeclarationADefinition() ?
                            RT_Definition : RT_Declaration;
                SymbolType symbolType;
                if (llvm::isa<clang::CXXMethodDecl>(fd)) {
                    if (llvm::isa<clang::CXXConstructorDecl>(fd))
                        symbolType = ST_Constructor;
                    else if (llvm::isa<clang::CXXDestructorDecl>(fd))
                        symbolType = ST_Destructor;
                    else
                        symbolType = ST_Method;
                } else {
                    symbolType = ST_Function;
                }
                RecordDeclRef(nd, loc, refType, symbolType);
            }
        } else if (clang::VarDecl *vd = llvm::dyn_cast<clang::VarDecl>(d)) {
            // Don't record the parameter definitions in a function declaration
            // (unless the function declaration is also a definition).  A
            // definition will be recorded at the function's definition, and
            // recording two definitions is unhelpful.  This code could record
            // a different kind of reference, but recording the position of
            // parameter names in declarations doesn't seem useful.
            bool omitParamVar = false;
            const bool isParam = llvm::isa<clang::ParmVarDecl>(vd);
            if (isParam) {
                clang::FunctionDecl *fd =
                        llvm::dyn_cast_or_null<clang::FunctionDecl>(
                            vd->getDeclContext());
                if (fd && !fd->isThisDeclarationADefinition())
                    omitParamVar = true;
            }
            if (!omitParamVar) {
                RefType refType;
                if (vd->isThisDeclarationADefinition() == clang::VarDecl::DeclarationOnly)
                    refType = RT_Declaration;
                else
                    refType = RT_Definition;
                // TODO: Review for correctness.  What about local extern?
                SymbolType symbolType;
                if (isParam)
                    symbolType = ST_Parameter;
                else if (vd->getParentFunctionOrMethod() == NULL)
                    symbolType = ST_GlobalVariable;
                else
                    symbolType = ST_LocalVariable;
                RecordDeclRef(nd, loc, refType, symbolType);
            }
        } else if (clang::TagDecl *td = llvm::dyn_cast<clang::TagDecl>(d)) {
            RefType refType;
            refType = td->isThisDeclarationADefinition() ? RT_Definition : RT_Declaration;
            // Mark an extern template declaration as a Declaration rather than
            // a Definition.  For example:
            //     template<typename T> class Foo {};  // Definition of Foo
            //     extern template class Foo<int>;     // Declaration of Foo
            if (clang::ClassTemplateSpecializationDecl *spec =
                    llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(td)) {
                if (spec->getTemplateSpecializationKind() !=
                        clang::TSK_ExplicitSpecialization)
                    refType = RT_Declaration;
            }
            // TODO: Handle the C++11 fixed underlying type of enumeration
            // declarations.
            SymbolType symbolType = ST_Max;
            switch (td->getTagKind()) {
            case clang::TTK_Struct:    symbolType = ST_Struct; break;
            case clang::TTK_Interface: symbolType = ST_Interface; break;
            case clang::TTK_Union:     symbolType = ST_Union; break;
            case clang::TTK_Class:     symbolType = ST_Class; break;
            case clang::TTK_Enum:      symbolType = ST_Enum; break;
            default: assert(false);
            }
            RecordDeclRef(nd, loc, refType, symbolType);
        } else if (clang::UsingDirectiveDecl *ud = llvm::dyn_cast<clang::UsingDirectiveDecl>(d)) {
            RecordDeclRef(
                        ud->getNominatedNamespaceAsWritten(),
                        loc, RT_UsingDirective);
        } else if (clang::UsingDecl *usd = llvm::dyn_cast<clang::UsingDecl>(d)) {
            for (auto it = usd->shadow_begin(), itEnd = usd->shadow_end();
                    it != itEnd; ++it) {
                clang::UsingShadowDecl *shadow = *it;
                RecordDeclRef(shadow->getTargetDecl(), loc, RT_Using);
            }
        } else if (clang::NamespaceAliasDecl *nad = llvm::dyn_cast<clang::NamespaceAliasDecl>(d)) {
            RecordDeclRef(nad, loc, RT_Declaration, ST_Namespace);
            RecordDeclRef(nad->getAliasedNamespace(),
                          nad->getTargetNameLoc(),
                          RT_NamespaceAlias);
        } else if (llvm::isa<clang::FunctionTemplateDecl>(d)) {
            // Do nothing.  The function will be recorded when it appears as a
            // FunctionDecl.
        } else if (llvm::isa<clang::ClassTemplateDecl>(d)) {
            // Do nothing.  The class will be recorded when it appears as a
            // RecordDecl.
        } else if (llvm::isa<clang::FieldDecl>(d)) {
            RecordDeclRef(nd, loc, RT_Declaration, ST_Field);
        } else if (llvm::isa<clang::TypedefDecl>(d)) {
            RecordDeclRef(nd, loc, RT_Declaration, ST_Typedef);
        } else if (llvm::isa<clang::NamespaceDecl>(d)) {
            RecordDeclRef(nd, loc, RT_Declaration, ST_Namespace);
        } else if (llvm::isa<clang::EnumConstantDecl>(d)) {
            RecordDeclRef(nd, loc, RT_Declaration, ST_Enumerator);
        } else {
            RecordDeclRef(nd, loc, RT_Declaration);
        }
    }

    return true;
}

bool ASTIndexer::VisitTypeLoc(clang::TypeLoc tl)
{
    if (!tl.getAs<clang::TagTypeLoc>().isNull()) {
        const clang::TagTypeLoc &ttl = tl.castAs<clang::TagTypeLoc>();
        RecordDeclRef(ttl.getDecl(),
                      tl.getBeginLoc(),
                      m_typeContext);
    } else if (!tl.getAs<clang::TypedefTypeLoc>().isNull()) {
        const clang::TypedefTypeLoc &ttl = tl.castAs<clang::TypedefTypeLoc>();
        RecordDeclRef(ttl.getTypedefNameDecl(),
                      tl.getBeginLoc(),
                      m_typeContext);
    } else if (!tl.getAs<clang::TemplateTypeParmTypeLoc>().isNull()) {
        const clang::TemplateTypeParmTypeLoc &ttptl =
           tl.castAs<clang::TemplateTypeParmTypeLoc>();
        RecordDeclRef(ttptl.getDecl(),
                      tl.getBeginLoc(),
                      m_typeContext);
    } else if (!tl.getAs<clang::TemplateSpecializationTypeLoc>().isNull()) {
        const clang::TemplateSpecializationTypeLoc &tstl =
           tl.castAs<clang::TemplateSpecializationTypeLoc>();
        const clang::TemplateSpecializationType &tst =
           *tstl.getTypePtr()->getAs<clang::TemplateSpecializationType>();
        if (tst.getAsCXXRecordDecl()) {
            RecordDeclRef(tst.getAsCXXRecordDecl(),
                          tl.getBeginLoc(),
                          m_typeContext);
        }
    }

    return true;
}


///////////////////////////////////////////////////////////////////////////////
// Reference recording

static inline bool isNamedDeclUnnamed(clang::NamedDecl *d)
{
    return d->getDeclName().isIdentifier() && d->getIdentifier() == NULL;
}

std::pair<Location, Location> ASTIndexer::getDeclRefRange(
        IndexerFileContext &fileContext,
        clang::NamedDecl *decl,
        clang::SourceLocation loc)
{
    clang::SourceLocation sloc =
            m_indexerContext.sourceManager().getSpellingLoc(loc);
    Location beginLoc = fileContext.location(sloc);
    clang::DeclarationName name = decl->getDeclName();
    clang::DeclarationName::NameKind nameKind = name.getNameKind();

    // A C++ destructor name consists of two tokens, '~' and an identifier.
    // Try to include both of them in the ref.
    if (nameKind == clang::DeclarationName::CXXDestructorName) {
        // Start by getting the destructor name, sans template arguments.
        const clang::Type *nameType = name.getCXXNameType().getTypePtr();
        assert(nameType != NULL);
        llvm::StringRef className;
        if (const clang::InjectedClassNameType *injectedNameType =
                nameType->getAs<clang::InjectedClassNameType>()) {
            className = injectedNameType->getDecl()->getName();
        } else if (const clang::RecordType *recordType =
                nameType->getAs<clang::RecordType>()) {
            className = recordType->getDecl()->getName();
        }
        if (!className.empty()) {
            // Scan the characters.
            const char *const buffer = m_indexerContext.sourceManager().getCharacterData(sloc);
            const char *p = buffer;
            if (p != NULL && *p == '~') {
                p++;
                // Permit whitespace between the ~ and the class name.
                // Technically there could be preprocessor tokens, comments,
                // etc..
                while (*p == ' ' || *p == '\t')
                    p++;
                // Match the class name against the text in the source.
                if (!strncmp(p, className.data(), className.size())) {
                    p += className.size();
                    if (!isalnum(*p) && *p != '_') {
                        Location endLoc = beginLoc;
                        endLoc.column += p - buffer;
                        return std::make_pair(beginLoc, endLoc);
                    }
                }
            }
        }
    }

    // For references to C++ overloaded operators, try to include both the
    // operator keyword and the operator name in the ref.
    if (nameKind == clang::DeclarationName::CXXOperatorName) {
        const char *spelling = clang::getOperatorSpelling(
                    name.getCXXOverloadedOperator());
        if (spelling != NULL) {
            const char *const buffer = m_indexerContext.sourceManager().getCharacterData(sloc);
            const char *p = buffer;
            if (p != NULL && !strncmp(p, "operator", 8)) {
                p += 8;
                // Skip whitespace between "operator" and the operator itself.
                while (*p == ' ' || *p == '\t')
                    p++;
                // Look for the operator name.  This may be too restrictive for
                // recognizing multi-token operators like operator[], operator
                // delete[], or operator ->*.
                if (!strncmp(p, spelling, strlen(spelling))) {
                    p += strlen(spelling);
                    Location endLoc = beginLoc;
                    endLoc.column += p - buffer;
                    return std::make_pair(beginLoc, endLoc);
                }
            }
        }
    }

    // General case -- find the end of the token starting at loc.
    clang::SourceLocation endSloc =
            m_indexerContext.preprocessor().getLocForEndOfToken(sloc);
    Location endLoc = fileContext.location(endSloc);
    return std::make_pair(beginLoc, endLoc);
}

void ASTIndexer::RecordDeclRef(
        clang::NamedDecl *d,
        clang::SourceLocation beginLoc,
        RefType refType,
        SymbolType symbolType)
{
    assert(d != NULL);

    // Skip references to unnamed declarations.  This is expected to skip the
    // definitions of unnamed types (structs/unions/enums).
    if (isNamedDeclUnnamed(d))
        return;

    beginLoc = m_indexerContext.sourceManager().getSpellingLoc(beginLoc);
    clang::FileID fileID;
    if (beginLoc.isValid())
        fileID = m_indexerContext.sourceManager().getFileID(beginLoc);
    IndexerFileContext &fileContext = m_indexerContext.fileContext(fileID);
    indexdb::ID symbolID = fileContext.getDeclSymbolID(d);
    std::pair<Location, Location> range = getDeclRefRange(fileContext, d, beginLoc);

    // Pass the prepared data to the IndexBuilder to record.
    fileContext.builder().recordRef(
                symbolID,
                range.first,
                range.second,
                fileContext.getRefTypeID(refType));
    if (symbolType != ST_Max) {
        fileContext.builder().recordSymbol(
                    symbolID,
                    fileContext.getSymbolTypeID(symbolType));
        if (symbolType != ST_LocalVariable && symbolType != ST_Parameter) {
            fileContext.builder().recordGlobalSymbol(symbolID);
        }
    }
}

} // namespace indexer
