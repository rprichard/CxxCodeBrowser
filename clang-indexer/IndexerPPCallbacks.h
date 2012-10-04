#ifndef INDEXER_INDEXERPPCALLBACKS_H
#define INDEXER_INDEXERPPCALLBACKS_H

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/MacroInfo.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Token.h>
#include <tuple>
#include <unordered_map>

#include "../libindexdb/IndexDb.h"
#include "Location.h"

namespace indexer {

class IndexerContext;
class IndexerFileContext;
enum RefType : int;

class IndexerPPCallbacks : public clang::PPCallbacks
{
public:
    IndexerPPCallbacks(IndexerContext &context);

private:
    virtual void InclusionDirective(clang::SourceLocation hashLoc,
                                    const clang::Token &includeTok,
                                    llvm::StringRef fileName,
                                    bool isAngled,
                                    const clang::FileEntry *file,
                                    clang::SourceLocation endLoc,
                                    llvm::StringRef searchPath,
                                    llvm::StringRef relativePath);
    std::tuple<IndexerFileContext*, Location, Location>
    getIncludeFilenameLoc(
            bool isAngled,
            clang::SourceLocation endLoc);
    virtual void MacroExpands(const clang::Token &macroNameToken,
                              const clang::MacroInfo *mi,
                              clang::SourceRange range);
    virtual void MacroDefined(const clang::Token &macroNameToken,
                              const clang::MacroInfo *mi);
    virtual void MacroUndefined(const clang::Token &macroNameTok,
                                const clang::MacroInfo *mi);
    virtual void Defined(const clang::Token &macroNameToken);
    virtual void Ifdef(clang::SourceLocation loc, const clang::Token &macroNameToken) { Defined(macroNameToken); }
    virtual void Ifndef(clang::SourceLocation loc, const clang::Token &macroNameToken) { Defined(macroNameToken); }

    void recordReference(const clang::Token &macroNameToken, RefType refType);

    IndexerContext &m_context;
    std::string m_tempSymbolName;
    std::unordered_map<const clang::FileEntry*, std::string> m_includePathMap;
};

} // namespace indexer

#endif // INDEXER_INDEXERPPCALLBACKS_H
