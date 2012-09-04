#ifndef INDEXER_INDEXERPPCALLBACKS_H
#define INDEXER_INDEXERPPCALLBACKS_H

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/MacroInfo.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Token.h>

namespace indexer {

struct IndexerPPCallbacks : clang::PPCallbacks
{
    clang::SourceManager *pSM;
    IndexerPPCallbacks(clang::SourceManager *pSM) : pSM(pSM) {}
    virtual void MacroExpands(const clang::Token &macroNameTok,
                              const clang::MacroInfo *MI,
                              clang::SourceRange Range);
    virtual void MacroDefined(const clang::Token &macroNameTok,
                              const clang::MacroInfo *MI);
    virtual void Defined(const clang::Token &macroNameTok);
    virtual void Ifdef(clang::SourceLocation Loc, const clang::Token &macroNameTok) { Defined(macroNameTok); }
    virtual void Ifndef(clang::SourceLocation Loc, const clang::Token &macroNameTok) { Defined(macroNameTok); }
};

} // namespace indexer

#endif // INDEXER_INDEXERPPCALLBACKS_H
