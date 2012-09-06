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
#include "IndexerContext.h"
#include "IndexerPPCallbacks.h"

namespace indexer {

class IndexerASTConsumer : public clang::ASTConsumer {
public:
    IndexerASTConsumer(IndexerContext &context) : m_context(context) {}

private:
    virtual bool HandleTopLevelDecl(clang::DeclGroupRef declGroup);

    IndexerContext m_context;
};

bool IndexerASTConsumer::HandleTopLevelDecl(clang::DeclGroupRef declGroup)
{
    for (clang::DeclGroupRef::iterator i = declGroup.begin(); i != declGroup.end(); ++i) {
        clang::Decl *decl = *i;
        ASTIndexer iv(m_context);
        iv.indexDecl(decl);
    }
    return true;
}

class IndexerAction : public clang::ASTFrontendAction {
public:
    IndexerAction(indexdb::Index &index) :
        m_index(index), m_context(NULL)
    {
    }

    ~IndexerAction()
    {
        delete m_context;
    }

private:
    IndexerContext &getContext(clang::CompilerInstance &ci) {
        if (m_context == NULL)
            m_context = new IndexerContext(ci.getSourceManager(), m_index);
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

    indexdb::Index &m_index;
    IndexerContext *m_context;
};

void indexTranslationUnit(
        const std::vector<std::string> &argv,
        indexdb::Index &index)
{
    llvm::OwningPtr<clang::FileManager> fm(new clang::FileManager(clang::FileSystemOptions()));
    llvm::OwningPtr<IndexerAction> action(new IndexerAction(index));
    clang::tooling::ToolInvocation ti(argv, action.take(), fm.get());
    ti.run();
}

} // namespace indexer
