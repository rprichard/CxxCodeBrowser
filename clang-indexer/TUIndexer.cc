#include "TUIndexer.h"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclGroup.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/FileSystemOptions.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Tooling/Tooling.h>
#include <iostream>
#include <llvm/ADT/StringRef.h>
#include <string>
#include <vector>

#include "../libindexdb/IndexDb.h"
#include "../libindexdb/IndexArchiveBuilder.h"
#include "ASTIndexer.h"
#include "IndexBuilder.h"
#include "IndexerContext.h"
#include "IndexerPPCallbacks.h"

namespace indexer {


///////////////////////////////////////////////////////////////////////////////
// IndexerASTConsumer

class IndexerASTConsumer : public clang::ASTConsumer {
public:
    IndexerASTConsumer(IndexerContext &context) : m_context(context) {}

private:
    void HandleTranslationUnit(clang::ASTContext &ctx);

    IndexerContext &m_context;
};

void IndexerASTConsumer::HandleTranslationUnit(clang::ASTContext &ctx)
{
    ASTIndexer iv(m_context);
    iv.indexDecl(ctx.getTranslationUnitDecl());
}


///////////////////////////////////////////////////////////////////////////////
// IndexerAction

class IndexerAction : public clang::ASTFrontendAction {
public:
    IndexerAction(indexdb::IndexArchiveBuilder &archive) :
        m_archive(archive), m_context(NULL)
    {
    }

    ~IndexerAction()
    {
        delete m_context;
    }

private:
    IndexerContext &getContext(clang::CompilerInstance &ci) {
        if (m_context == NULL) {
            m_context = new IndexerContext(
                        ci.getSourceManager(),
                        ci.getPreprocessor(),
                        m_archive);
        }
        return *m_context;
    }

    virtual clang::ASTConsumer *CreateASTConsumer(
            clang::CompilerInstance &ci,
            llvm::StringRef inFile) {
        return new IndexerASTConsumer(getContext(ci));
    }

    virtual bool BeginSourceFileAction(clang::CompilerInstance &ci,
                                       llvm::StringRef filename) {
        ci.getDiagnostics().setClient(new clang::IgnoringDiagConsumer);
        ci.getPreprocessor().addPPCallbacks(
                    new IndexerPPCallbacks(getContext(ci)));
        return true;
    }

    indexdb::IndexArchiveBuilder &m_archive;
    IndexerContext *m_context;
};


///////////////////////////////////////////////////////////////////////////////
// indexTranslationUnit

void indexTranslationUnit(
        const std::vector<std::string> &argv,
        indexdb::IndexArchiveBuilder &archive)
{
    clang::FileManager *fm = new clang::FileManager(clang::FileSystemOptions());
    IndexerAction *action = new IndexerAction(archive);
    clang::tooling::ToolInvocation ti(argv, action, fm);
    ti.run();
}

} // namespace indexer
