#ifndef INDEXER_ASTINDEXER_H
#define INDEXER_ASTINDEXER_H

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceManager.h>

#include "Location.h"

namespace indexer {

class IndexBuilder;

class ASTIndexer : clang::RecursiveASTVisitor<ASTIndexer>
{
public:
    ASTIndexer(clang::SourceManager *pSM, IndexBuilder &builder) :
        m_pSM(pSM), m_builder(builder), m_thisContext(0), m_childContext(0)
    {
    }

    void indexDecl(clang::Decl *d) { TraverseDecl(d); }

private:
    typedef clang::RecursiveASTVisitor<ASTIndexer> base;
    friend class clang::RecursiveASTVisitor<ASTIndexer>;

    // XXX: The CF_Read flag is useful mostly for lvalues -- for rvalues, we
    // don't set the CF_Read flag, but the rvalue is assumed to be read anyway.

    enum ContextFlags {
        CF_Called       = 0x1,      // the value is read only to be called
        CF_Read         = 0x2,      // the value is read for any other use
        CF_AddressTaken = 0x4,      // the gl-value's address escapes
        CF_Assigned     = 0x8,      // the gl-value is assigned to
        CF_Mutated      = 0x10      // the gl-value is updated (i.e. compound assignment)
    };

    typedef unsigned int Context;

    clang::SourceManager *m_pSM;
    IndexBuilder &m_builder;
    Context m_thisContext;
    Context m_childContext;

    // Misc routines
    bool shouldUseDataRecursionFor(clang::Stmt *s) const;

    // Dispatcher routines
    bool TraverseStmt(clang::Stmt *stmt);
    bool TraverseType(clang::QualType t);
    bool TraverseTypeLoc(clang::TypeLoc tl);
    bool TraverseDecl(clang::Decl *d);

    // Expression context propagation
    bool TraverseCallExpr(clang::CallExpr *e) { return TraverseCallCommon(e); }
    bool TraverseCXXMemberCallExpr(clang::CXXMemberCallExpr *e) { return TraverseCallCommon(e); }
    bool TraverseCXXOperatorCallExpr(clang::CXXOperatorCallExpr *e) { return TraverseCallCommon(e); }
    bool TraverseBinComma(clang::BinaryOperator *s);
    bool TraverseBinAssign(clang::BinaryOperator *e) { return TraverseAssignCommon(e, CF_Assigned); }
#define OPERATOR(NAME) bool TraverseBin##NAME##Assign(clang::CompoundAssignOperator *e) { return TraverseAssignCommon(e, CF_Mutated); }
    OPERATOR(Mul) OPERATOR(Div) OPERATOR(Rem) OPERATOR(Add) OPERATOR(Sub)
    OPERATOR(Shl) OPERATOR(Shr) OPERATOR(And) OPERATOR(Or)  OPERATOR(Xor)
#undef OPERATOR
    bool VisitParenExpr(clang::ParenExpr *e) { m_childContext = m_thisContext; return true; }
    bool VisitCastExpr(clang::CastExpr *e);
    bool VisitUnaryAddrOf(clang::UnaryOperator *e);
    bool VisitUnaryDeref(clang::UnaryOperator *e);
    bool VisitDeclStmt(clang::DeclStmt *s);
    bool VisitReturnStmt(clang::ReturnStmt *s);
    bool VisitVarDecl(clang::VarDecl *d);
    bool VisitInitListExpr(clang::InitListExpr *e);
    bool TraverseConstructorInitializer(clang::CXXCtorInitializer *init);
    bool TraverseCallCommon(clang::CallExpr *call);
    bool TraverseAssignCommon(clang::BinaryOperator *e, ContextFlags lhsFlag);

    // Expression reference recording
    bool VisitMemberExpr(clang::MemberExpr *e);
    bool VisitDeclRefExpr(clang::DeclRefExpr *e);
    void RecordDeclRefExpr(clang::NamedDecl *d, const Location &loc, clang::Expr *e, Context context);

    // NestedNameSpecifier handling
    bool TraverseNestedNameSpecifierLoc(clang::NestedNameSpecifierLoc qualifier);

    // Declaration handling
    bool VisitDecl(clang::Decl *d);

    // Reference recording
    void RecordDeclRef(clang::NamedDecl *d, const Location &loc, const char *kind);
};

} // namespace indexer

#endif // INDEXER_ASTINDEXER_H
