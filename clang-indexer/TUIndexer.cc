#include "TUIndexer.h"

#include <clang/AST/ASTConsumer.h>
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
#include "ASTIndexer.h"
#include "IndexBuilder.h"
#include "IndexerPPCallbacks.h"

namespace indexer {

class IndexerASTConsumer : public clang::ASTConsumer {
public:
    IndexerASTConsumer(
            clang::SourceManager *pSM,
            IndexBuilder &builder) :
        m_pSM(pSM), m_builder(builder)
    {
    }

private:
    virtual bool HandleTopLevelDecl(clang::DeclGroupRef declGroup);

    clang::SourceManager *m_pSM;
    IndexBuilder &m_builder;
};

bool IndexerASTConsumer::HandleTopLevelDecl(clang::DeclGroupRef declGroup)
{
    for (clang::DeclGroupRef::iterator i = declGroup.begin(); i != declGroup.end(); ++i) {
        clang::Decl *decl = *i;
        ASTIndexer iv(m_pSM, m_builder);
        iv.indexDecl(decl);
    }
    return true;
}

class IndexerAction : public clang::ASTFrontendAction {
public:
    IndexerAction(IndexBuilder &builder) : m_builder(builder) {}

private:
    virtual clang::ASTConsumer *CreateASTConsumer(
            clang::CompilerInstance &ci,
            llvm::StringRef inFile) {
        IndexerASTConsumer *astConsumer =
                new IndexerASTConsumer(&ci.getSourceManager(), m_builder);
        return astConsumer;
    }

    virtual bool BeginSourceFileAction(clang::CompilerInstance &ci,
                                       llvm::StringRef filename) {
        ci.getDiagnostics().setClient(new clang::IgnoringDiagConsumer);
        ci.getPreprocessor().addPPCallbacks(
                    new IndexerPPCallbacks(&ci.getSourceManager(), m_builder));
        return true;
    }

    IndexBuilder &m_builder;
};

void indexTranslationUnit(
        const std::vector<std::string> &argv,
        indexdb::Index *index)
{
    IndexBuilder builder(index);

    llvm::OwningPtr<clang::FileManager> fm(new clang::FileManager(clang::FileSystemOptions()));
    llvm::OwningPtr<IndexerAction> action(new IndexerAction(builder));
    clang::tooling::ToolInvocation ti(argv, action.take(), fm.get());
    ti.run();
}

} // namespace indexer
