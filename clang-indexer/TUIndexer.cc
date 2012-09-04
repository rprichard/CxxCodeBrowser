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

#include "ASTIndexer.h"
#include "IndexerPPCallbacks.h"

namespace indexer {

struct IndexerASTConsumer : clang::ASTConsumer {
    clang::SourceManager *pSM;

    virtual bool HandleTopLevelDecl(clang::DeclGroupRef declGroup);
};

bool IndexerASTConsumer::HandleTopLevelDecl(clang::DeclGroupRef declGroup)
{
    for (clang::DeclGroupRef::iterator i = declGroup.begin(); i != declGroup.end(); ++i) {
        std::cerr << "=====HandleTopLevelDecl" << std::endl;
        clang::Decl *decl = *i;

        ASTIndexer iv(pSM);
        iv.indexDecl(decl);
    }
    return true;
}

struct IndexerAction : clang::ASTFrontendAction {
    virtual clang::ASTConsumer *CreateASTConsumer(
            clang::CompilerInstance &ci,
            llvm::StringRef inFile) {
        IndexerASTConsumer *astConsumer = new IndexerASTConsumer;
        astConsumer->pSM = &ci.getSourceManager();
        return astConsumer;
    }
    virtual bool BeginSourceFileAction(clang::CompilerInstance &ci,
                                       llvm::StringRef filename) {
        ci.getDiagnostics().setClient(new clang::IgnoringDiagConsumer);
        ci.getPreprocessor().addPPCallbacks(new IndexerPPCallbacks(&ci.getSourceManager()));
        return true;
    }
};

void indexTranslationUnit(const std::vector<std::string> &argv)
{
    /*
    std::vector<std::string> argv;
    argv.push_back("/home/rprichard/llvm-install-dbg/bin/clang++");
    argv.push_back("-c");
    argv.push_back("-std=c++11");
    argv.push_back("test.cc");
    */
    llvm::OwningPtr<clang::FileManager> fm(new clang::FileManager(clang::FileSystemOptions()));
    llvm::OwningPtr<IndexerAction> action(new IndexerAction);
    clang::tooling::ToolInvocation ti(argv, action.take(), fm.get());
    ti.run();
}

} // namespace indexer
