#include <stdio.h>
#include <iostream>
#include "llvm/Support/Host.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Parse/Parser.h"
#include "clang/Parse/ParseAST.h"


static clang::SourceManager *pSM;
static int includeLevel = 0;


static void indent()
{
    for (int i = 0; i < includeLevel; ++i)
        std::cerr << "|       ";
}


struct MyPPCallbacks : clang::PPCallbacks
{
    virtual void MacroExpands(const clang::Token &macroNameTok,
                              const clang::MacroInfo *MI,
                              clang::SourceRange Range);

    virtual void MacroDefined(const clang::Token &macroNameTok,
                              const clang::MacroInfo *MI);

    virtual void FileChanged(clang::SourceLocation Loc,
                             clang::PPCallbacks::FileChangeReason Reason,
                             clang::SrcMgr::CharacteristicKind FileType,
                             clang::FileID PrevFID);
};

void MyPPCallbacks::MacroExpands(const clang::Token &macroNameTok,
                                 const clang::MacroInfo *MI,
                                 clang::SourceRange Range)
{
    return;

    indent();

    std::cerr << "expand " << macroNameTok.getIdentifierInfo()->getName().str() << std::endl;

    for (clang::MacroInfo::tokens_iterator it = MI->tokens_begin();
         it != MI->tokens_end();
         ++it) {
        indent();
        std::cerr << "   " << it->getName() << std::endl;
    }

}

void MyPPCallbacks::MacroDefined(const clang::Token &macroNameTok,
                                 const clang::MacroInfo *MI)
{
    indent();

    clang::SourceLocation loc = macroNameTok.getLocation();

    std::cerr << pSM->getSpellingLineNumber(loc)
              << ":"
              << pSM->getSpellingColumnNumber(loc)
              << ": #define "
              << macroNameTok.getIdentifierInfo()->getName().str()
              << std::endl;
}

void MyPPCallbacks::FileChanged(clang::SourceLocation loc,
                                clang::PPCallbacks::FileChangeReason reason,
                                clang::SrcMgr::CharacteristicKind fileType,
                                clang::FileID prevFID)
{
    if (reason == clang::PPCallbacks::EnterFile) {
        indent();
        std::cerr << "enter " << pSM->getBufferName(loc)
                  << "(" << pSM->getFileID(loc).getHashValue() << ")"
                  << std::endl;

        const clang::FileEntry *pFE = pSM->getFileEntryForID(pSM->getFileID(loc));
        if (pFE) {
            llvm::MemoryBuffer *mem = pSM->getFileManager().getBufferForFile(pFE);
            assert(mem);
            llvm::StringRef str = mem->getBuffer();

            indent();
            std::string piece(str.data(), 10);
            std::cerr << piece << std::endl;
        }
        indent();

        ++includeLevel;
    } else if (reason == clang::PPCallbacks::ExitFile) {
        --includeLevel;
        indent();
        std::cerr << "exit" << std::endl;
    }
}

int main()
{
    using clang::FileEntry;
    using clang::Token;
    using clang::ASTContext;
    using clang::ASTConsumer;
    using clang::Parser;

    clang::CompilerInstance ci;
    ci.createDiagnostics(0, NULL);

    clang::TargetOptions to;
    to.Triple = llvm::sys::getDefaultTargetTriple();
    clang::TargetInfo *pti = clang::TargetInfo::CreateTargetInfo(ci.getDiagnostics(), to);
    ci.setTarget(pti);

    clang::HeaderSearchOptions &hso = ci.getHeaderSearchOpts();
    const char *paths[] = {
        "/usr/include/c++/4.6",
        "/usr/include/c++/4.6/x86_64-linux-gnu/.",
        "/usr/include/c++/4.6/backward",
        "/usr/local/include",
        "/usr/include/x86_64-linux-gnu",
        "/usr/include",
        "/usr/include/clang/3.0/include/",
        "/usr/lib/gcc/x86_64-linux-gnu/4.6/include",
        "/usr/lib/gcc/x86_64-linux-gnu/4.6/include-fixed",
    };
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
        hso.AddPath(paths[i], clang::frontend::System, false, false, false);
    }






    ci.createFileManager();
    ci.createSourceManager(ci.getFileManager());
    ci.getPreprocessorOpts().UsePredefines = true;
    ci.createPreprocessor();
    ASTConsumer *astConsumer = new ASTConsumer();
    ci.setASTConsumer(astConsumer);

    pSM = &ci.getSourceManager();


    ci.getPreprocessor().addPPCallbacks(new MyPPCallbacks());



    ci.createASTContext();
    //ci.createSema(clang::TU_Complete, NULL);
    //ci.getSema().Initialize();

    const FileEntry *pFile = ci.getFileManager().getFile("/home/rprichard/project/test.c");
    ci.getSourceManager().createMainFileID(pFile);

    //std::cerr << ci.getPreprocessor().getPredefines() << std::endl;

    //ci.getPreprocessor().EnterMainSourceFile();
    ci.getDiagnosticClient().BeginSourceFile(ci.getLangOpts(),
                                             &ci.getPreprocessor());

#if 0
    Token tok;
    do {
        ci.getPreprocessor().Lex(tok);
        if (ci.getDiagnostics().hasErrorOccurred()) {
            std::cerr << "err occurred!" << std::endl;
            break;
        }
        ci.getPreprocessor().DumpToken(tok);
        std::cerr << std::endl;
    } while (tok.isNot(clang::tok::eof));
#endif


    //Parser parser(ci.getPreprocessor(), astConsumer, false);
    //parser.ParseTranslationUnit();

    clang::ParseAST(ci.getPreprocessor(), astConsumer, ci.getASTContext());

    ci.getDiagnosticClient().EndSourceFile();

    //ci.getASTContext().Idents.PrintStats();




    return 0;
}
