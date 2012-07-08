#include "SourceIndexer.h"
#include "Project.h"
#include "CSource.h"
#include "Symbol.h"
#include "SymbolTable.h"
#include "FileManager.h"
#include "Ref.h"
#include <QDebug>
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

namespace Nav {

struct IndexerPPCallbacks : clang::PPCallbacks
{
    FileManager *pFM;
    SymbolTable *pST;
    clang::SourceManager *pSM;

    virtual void MacroExpands(
            const clang::Token &macroNameTok,
            const clang::MacroInfo *mi,
            clang::SourceRange range);

    virtual void MacroDefined(
            const clang::Token &macroNameTok,
            const clang::MacroInfo *mi);

    virtual void FileChanged(
            clang::SourceLocation Loc,
            clang::PPCallbacks::FileChangeReason reason,
            clang::SrcMgr::CharacteristicKind fileType,
            clang::FileID prevFID);
};

void IndexerPPCallbacks::MacroExpands(
        const clang::Token &macroNameTok,
        const clang::MacroInfo *mi,
        clang::SourceRange range)
{
    clang::SourceLocation loc = range.getBegin();
    QString name = QString::fromStdString(
                macroNameTok.getIdentifierInfo()->getName().str());
    Symbol *symbol = pST->symbol(name, true);
    Ref ref;
    ref.symbol = symbol;
    ref.file = pFM->file(pSM->getBufferName(loc));
    ref.line = pSM->getSpellingLineNumber(loc);
    ref.column = pSM->getSpellingColumnNumber(loc);
    ref.kind = "Usage";
    // TODO: Do something about null files.
    if (ref.file != NULL)
        symbol->refs.insert(ref);
}

void IndexerPPCallbacks::MacroDefined(const clang::Token &macroNameTok,
                                 const clang::MacroInfo *mi)
{
    clang::SourceLocation loc = macroNameTok.getLocation();
    QString name = QString::fromStdString(
                macroNameTok.getIdentifierInfo()->getName().str());
    Symbol *symbol = pST->symbol(name, true);
    Ref ref;
    ref.symbol = symbol;
    ref.file = pFM->file(pSM->getBufferName(loc));
    ref.line = pSM->getSpellingLineNumber(loc);
    ref.column = pSM->getSpellingColumnNumber(loc);
    ref.kind = "Definition";
    // TODO: Do something about null files.
    if (ref.file != NULL)
        symbol->refs.insert(ref);
}

void IndexerPPCallbacks::FileChanged(clang::SourceLocation loc,
                                clang::PPCallbacks::FileChangeReason reason,
                                clang::SrcMgr::CharacteristicKind fileType,
                                clang::FileID prevFID)
{
}










void indexCSource(Project *project, CSource *csource)
{
    qDebug() << "Indexing" << csource->path;

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


    IndexerPPCallbacks *callbacks = new IndexerPPCallbacks();
    callbacks->pSM = &ci.getSourceManager();
    callbacks->pFM = project->fileManager;
    callbacks->pST = project->symbolTable;
    ci.getPreprocessor().addPPCallbacks(callbacks);



    ci.createASTContext();
    //ci.createSema(clang::TU_Complete, NULL);
    //ci.getSema().Initialize();

    const FileEntry *pFile = ci.getFileManager().getFile(csource->path.toStdString().c_str());
    ci.getSourceManager().createMainFileID(pFile);

    //std::cerr << ci.getPreprocessor().getPredefines() << std::endl;

    //ci.getPreprocessor().EnterMainSourceFile();
    ci.getDiagnosticClient().BeginSourceFile(ci.getLangOpts(),
                                             &ci.getPreprocessor());

    //Parser parser(ci.getPreprocessor(), astConsumer, false);
    //parser.ParseTranslationUnit();

    clang::ParseAST(ci.getPreprocessor(), astConsumer, ci.getASTContext());

    ci.getDiagnosticClient().EndSourceFile();
}

void indexProject(Project *project)
{
    foreach (CSource *csource, project->csources) {
        indexCSource(project, csource);
    }
}

} // namespace Nav
