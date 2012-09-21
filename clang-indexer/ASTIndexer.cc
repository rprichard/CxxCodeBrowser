#include "ASTIndexer.h"

#include <clang/AST/Type.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/Casting.h>
#include <string>

#include "IndexBuilder.h"
#include "IndexerContext.h"
#include "NameGenerator.h"
#include "Switcher.h"
#include "USRGenerator.h"

namespace indexer {

// TODO: For an array access, X[I], skip over the array-to-pointer decay.  We
// currently record that X's address is taken, which is technically true, but
// the address does not generally escape.

// TODO: A struct assignment in C++ turns into an operator= call, even if the
// user did not define an operator= function.  Consider handling operator=
// calls the same way that normal assignment operations are handled.  Consider
// doing the same for the other overloadable operators.

// TODO: Unary ++ and --
// TODO: templates

///////////////////////////////////////////////////////////////////////////////
// Misc routines

bool ASTIndexer::shouldUseDataRecursionFor(clang::Stmt *s) const
{
    if (s == NULL || !base::shouldUseDataRecursionFor(s))
        return false;
    if (clang::UnaryOperator *e = llvm::dyn_cast<clang::UnaryOperator>(s)) {
        return e->getOpcode() >= clang::UO_Plus &&
                e->getOpcode() <= clang::UO_Imag;
    }
    if (clang::BinaryOperator *e = llvm::dyn_cast<clang::BinaryOperator>(s)) {
        return e->getOpcode() >= clang::BO_Mul &&
                e->getOpcode() <= clang::BO_LOr;
    }
    return false;
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
            m_thisContext &= ~(CF_AddressTaken | CF_Assigned | CF_Mutated);
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
                      "Initialized");
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
        if (m_thisContext & (CF_Assigned | CF_Mutated))
            m_childContext |= CF_Mutated;
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

void ASTIndexer::RecordDeclRefExpr(clang::NamedDecl *d, clang::SourceLocation loc, clang::Expr *e, Context context)
{
    if (llvm::isa<clang::FunctionDecl>(*d)) {
        // XXX: This code seems sloppy, but I suspect it will work well enough.
        if (context & CF_Called)
            RecordDeclRef(d, loc, "Called");
        if (!(context & CF_Called) || (context & (CF_Read | CF_AddressTaken)))
            RecordDeclRef(d, loc, "Address-Taken");
    } else {
        if (context & CF_Called)
            RecordDeclRef(d, loc, "Called");
        if (context & CF_Read)
            RecordDeclRef(d, loc, "Read");
        if (context & CF_AddressTaken)
            RecordDeclRef(d, loc, "Address-Taken");
        if (context & CF_Assigned)
            RecordDeclRef(d, loc, "Assigned");
        if (context & CF_Mutated)
            RecordDeclRef(d, loc, "Modified");
        if (context == 0)
            RecordDeclRef(d, loc, e->isRValue() ? "Read" : "Other");
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
                          "Qualifier");
            break;
        case clang::NestedNameSpecifier::NamespaceAlias:
            RecordDeclRef(nns->getAsNamespaceAlias(),
                          qualifier.getLocalBeginLoc(),
                          "Qualifier");
            break;
        case clang::NestedNameSpecifier::TypeSpec:
        case clang::NestedNameSpecifier::TypeSpecWithTemplate:
            if (const clang::TypedefType *tt = nns->getAsType()->getAs<clang::TypedefType>()) {
                RecordDeclRef(tt->getDecl(),
                              qualifier.getLocalBeginLoc(),
                              "Qualifier");
            } else if (const clang::RecordType *rt = nns->getAsType()->getAs<clang::RecordType>()) {
                RecordDeclRef(rt->getDecl(),
                              qualifier.getLocalBeginLoc(),
                              "Qualifier");
            } else if (const clang::TemplateSpecializationType *tst =
                       nns->getAsType()->getAs<clang::TemplateSpecializationType>()) {

                if (clang::TemplateDecl *decl = tst->getTemplateName().getAsTemplateDecl()) {
                    if (clang::NamedDecl *templatedDecl = decl->getTemplatedDecl()) {
                        RecordDeclRef(templatedDecl,
                                      qualifier.getLocalBeginLoc(),
                                      "Qualifier");

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
            Switcher<const char*> sw(m_typeContext, "Base-Class");
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
                const char *kind;
                kind = fd->isThisDeclarationADefinition() ? "Definition" : "Declaration";
                RecordDeclRef(nd, loc, kind);
            }
        } else if (clang::VarDecl *vd = llvm::dyn_cast<clang::VarDecl>(d)) {
            // Don't record the parameter definitions in a function declaration
            // (unless the function declaration is also a definition).  A
            // definition will be recorded at the function's definition, and
            // recording two definitions is unhelpful.  This code could record
            // a different kind of reference, but recording the position of
            // parameter names in declarations doesn't seem useful.
            bool omitParmVar = false;
            clang::ParmVarDecl *pvd = llvm::dyn_cast<clang::ParmVarDecl>(d);
            if (pvd) {
                clang::FunctionDecl *fd =
                        llvm::dyn_cast_or_null<clang::FunctionDecl>(
                            pvd->getDeclContext());
                if (fd && !fd->isThisDeclarationADefinition())
                    omitParmVar = true;
            }
            if (!omitParmVar) {
                const char *kind;
                if (vd->isThisDeclarationADefinition() == clang::VarDecl::DeclarationOnly) {
                    kind = "Declaration";
                } else {
                    kind = "Definition";
                }
                RecordDeclRef(nd, loc, kind);
            }
        } else if (clang::TagDecl *td = llvm::dyn_cast<clang::TagDecl>(d)) {
            // TODO: Handle the C++11 fixed underlying type of enumeration
            // declarations.
            const char *kind;
            kind = td->isThisDeclarationADefinition() ? "Definition" : "Declaration";
            RecordDeclRef(nd, loc, kind);
        } else if (clang::UsingDirectiveDecl *ud = llvm::dyn_cast<clang::UsingDirectiveDecl>(d)) {
            RecordDeclRef(
                        ud->getNominatedNamespaceAsWritten(),
                        loc, "Using-Directive");
        } else if (clang::UsingDecl *usd = llvm::dyn_cast<clang::UsingDecl>(d)) {
            for (auto it = usd->shadow_begin(), itEnd = usd->shadow_end();
                    it != itEnd; ++it) {
                clang::UsingShadowDecl *shadow = *it;
                RecordDeclRef(shadow->getTargetDecl(), loc, "Using");
            }
        } else if (clang::NamespaceAliasDecl *nad = llvm::dyn_cast<clang::NamespaceAliasDecl>(d)) {
            RecordDeclRef(nad, loc, "Declaration");
            RecordDeclRef(nad->getAliasedNamespace(),
                          nad->getTargetNameLoc(),
                          "Namespace-Alias");
        } else if (llvm::isa<clang::FunctionTemplateDecl>(d)) {
            // Do nothing.  The function will be recorded when it appears as a
            // FunctionDecl.
        } else if (llvm::isa<clang::ClassTemplateDecl>(d)) {
            // Do nothing.  The class will be recorded when it appears as a
            // RecordDecl.
        } else {
            RecordDeclRef(nd, loc, "Declaration");
        }
    }

    return true;
}

bool ASTIndexer::VisitTypeLoc(clang::TypeLoc tl)
{
    if (llvm::isa<clang::TagTypeLoc>(tl)) {
        clang::TagTypeLoc &ttl = *llvm::cast<clang::TagTypeLoc>(&tl);
        RecordDeclRef(ttl.getDecl(),
                      tl.getBeginLoc(),
                      m_typeContext);
    } else if (llvm::isa<clang::TypedefTypeLoc>(tl)) {
        clang::TypedefTypeLoc &ttl = *llvm::cast<clang::TypedefTypeLoc>(&tl);
        RecordDeclRef(ttl.getTypedefNameDecl(),
                      tl.getBeginLoc(),
                      m_typeContext);
    } else if (llvm::isa<clang::TemplateTypeParmTypeLoc>(tl)) {
        clang::TemplateTypeParmTypeLoc &ttptl = *llvm::cast<clang::TemplateTypeParmTypeLoc>(&tl);
        RecordDeclRef(ttptl.getDecl(),
                      tl.getBeginLoc(),
                      m_typeContext);
    } else if (llvm::isa<clang::TemplateSpecializationTypeLoc>(tl)) {
        clang::TemplateSpecializationTypeLoc &tstl = *llvm::cast<clang::TemplateSpecializationTypeLoc>(&tl);
        const clang::TemplateSpecializationType &tst = *tstl.getTypePtr()->getAs<clang::TemplateSpecializationType>();
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

static bool isNamedDeclUnnamed(clang::NamedDecl *d)
{
    return d->getDeclName().isIdentifier() && d->getIdentifier() == NULL;
}

void ASTIndexer::RecordDeclRef(clang::NamedDecl *d, clang::SourceLocation loc, const char *kind)
{
    // Skip references to unnamed declarations.  This is expected to skip the
    // definitions of unnamed types (structs/unions/enums).
    if (isNamedDeclUnnamed(d))
        return;

    Location convertedLoc =
            m_indexerContext.locationConverter().convert(loc);
    std::string symbol;
    getDeclName(d, symbol);
    m_indexerContext.indexBuilder().recordRef(symbol.c_str(), convertedLoc, kind);
}

} // namespace indexer
