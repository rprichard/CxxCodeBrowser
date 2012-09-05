#include "ASTIndexer.h"

#include <iostream>
#include <llvm/Support/Casting.h>
#include <string>

#include "IndexBuilder.h"
#include "Switcher.h"

namespace indexer {

// TODO: For an array access, X[I], skip over the array-to-pointer decay.  We
// currently record that X's address is taken, which is technically true, but
// the address does not generally escape.

// TODO: A struct assignment in C++ turns into an operator= call, even if the
// user did not define an operator= function.  Consider handling operator=
// calls the same way that normal assignment operations are handled.  Consider
// doing the same for the other overloadable operators.

// TODO: Look at clang::UsingDecl and clang::UsingShadowDecl.  Consider this
// test case:
//     namespace Foo { enum E { Test }; E v2; }
//     using Foo::E;
//     E v1;
// Are the first "E" and the last "E" the same symbol in the index?  If there
// is a Foo::E identifier recorded, is a plain E also recorded?  Since they're
// actually the same type, I would think that an xref on one should show both.

// TODO: Unary ++ and --

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
                      convertLocation(m_pSM, init->getMemberLocation()),
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
                convertLocation(m_pSM, e->getMemberLoc()),
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
            m_childContext |= CF_Called;
        }
    }

    return true;
}

bool ASTIndexer::VisitDeclRefExpr(clang::DeclRefExpr *e)
{
    RecordDeclRefExpr(
                e->getDecl(),
                convertLocation(m_pSM, e->getLocation()),
                e,
                m_thisContext);
    return true;
}

void ASTIndexer::RecordDeclRefExpr(clang::NamedDecl *d, const Location &loc, clang::Expr *e, Context context)
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
                          convertLocation(m_pSM, qualifier.getLocalBeginLoc()),
                          "Qualifier");
            break;
        case clang::NestedNameSpecifier::NamespaceAlias:
            RecordDeclRef(nns->getAsNamespaceAlias(),
                          convertLocation(m_pSM, qualifier.getLocalBeginLoc()),
                          "Qualifier");
            break;
        case clang::NestedNameSpecifier::TypeSpec:
        case clang::NestedNameSpecifier::TypeSpecWithTemplate:
            {
                const clang::RecordType *rt = nns->getAsType()->getAs<clang::RecordType>();
                if (rt != NULL) {
                    RecordDeclRef(rt->getDecl(),
                                  convertLocation(m_pSM, qualifier.getLocalBeginLoc()),
                                  "Qualifier");
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
// Declaration handling

bool ASTIndexer::VisitDecl(clang::Decl *d)
{
    if (clang::NamedDecl *nd = llvm::dyn_cast<clang::NamedDecl>(d)) {
        Location loc = convertLocation(m_pSM, nd->getLocation());
        if (clang::FunctionDecl *fd = llvm::dyn_cast<clang::FunctionDecl>(d)) {
            const char *kind;
            kind = fd->isThisDeclarationADefinition() ? "Definition" : "Declaration";
            RecordDeclRef(nd, loc, kind);
        } else if (clang::VarDecl *vd = llvm::dyn_cast<clang::VarDecl>(d)) {
            const char *kind;
            if (vd->isThisDeclarationADefinition() == clang::VarDecl::DeclarationOnly) {
                kind = "Declaration";
            } else {
                kind = "Definition";
            }
            RecordDeclRef(nd, loc, kind);
        } else if (clang::CXXRecordDecl *rd = llvm::dyn_cast<clang::CXXRecordDecl>(d)) {
            const char *kind = "Declaration";
            if (rd->isThisDeclarationADefinition()) {
                kind = "Definition";
                // Record base classes.
                for (clang::CXXRecordDecl::base_class_iterator it = rd->bases_begin(); it != rd->bases_end(); ++it) {
                    clang::CXXBaseSpecifier *bs = it;
                    const clang::RecordType *baseType = bs->getType().getTypePtr()->getAs<clang::RecordType>();
                    if (baseType) {
                        RecordDeclRef(
                                    baseType->getDecl(),
                                    convertLocation(m_pSM, bs->getTypeSourceInfo()->getTypeLoc().getBeginLoc()),
                                    "Base-Class");
                    }
                }
            }
            RecordDeclRef(nd, loc, kind);
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
        } else {
            RecordDeclRef(nd, loc, "Declaration");
        }
    }

    return true;
}


///////////////////////////////////////////////////////////////////////////////
// Reference recording

void ASTIndexer::RecordDeclRef(clang::NamedDecl *d, const Location &loc, const char *kind)
{
    std::string name = d->getDeclName().getAsString();
    //std::cerr << loc.toString() << "," << kind << ": " << name << std::endl;

    // TODO: Use a usr.  The identifier by itself is not good enough.
    m_builder.recordRef(name.c_str(), loc, kind);
}

} // namespace indexer
