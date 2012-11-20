#include "IndexerPPCallbacks.h"

#include <clang/Basic/IdentifierTable.h>
#include <clang/Lex/Preprocessor.h>
#include <iostream>

#include "IndexBuilder.h"
#include "IndexerContext.h"
#include "Location.h"

namespace indexer {

IndexerPPCallbacks::IndexerPPCallbacks(IndexerContext &context) :
    m_context(context)
{
}

void IndexerPPCallbacks::InclusionDirective(
        clang::SourceLocation hashLoc,
        const clang::Token &includeTok,
        llvm::StringRef fileName,
        bool isAngled,
        const clang::FileEntry *file,
        clang::SourceLocation endLoc,
        llvm::StringRef searchPath,
        llvm::StringRef relativePath)
{
    if (file == NULL) {
        // The file can be NULL, in which case there is nothing for the indexer
        // to record.  (For example, the target of an #include might not be
        // found.)
        return;
    }

    // Get the location of the #include filename.
    auto range = getIncludeFilenameLoc(isAngled, endLoc);
    IndexerFileContext &fileContext = *std::get<0>(range);

    // Get the path ID of the included file.
    indexdb::ID symbolID;
    auto it = m_includePathMap.find(file);
    if (it == m_includePathMap.end()) {
        std::string symbol = "@";
        char *path = realpath(file->getName(), NULL);
        symbol += path;
        free(path);
        m_includePathMap[file] = symbol;
        symbolID = fileContext.builder().insertSymbol(symbol.c_str());
    } else {
        symbolID = fileContext.builder().insertSymbol(it->second.c_str());
    }

    // Insert the ref.
    fileContext.builder().recordRef(
                symbolID,
                std::get<1>(range),
                std::get<2>(range),
                fileContext.getRefTypeID(RT_Included));
}

std::tuple<IndexerFileContext*, Location, Location>
IndexerPPCallbacks::getIncludeFilenameLoc(
        bool isAngled,
        clang::SourceLocation endLoc)
{
    // The obvious thing to do is to associate the inclusion with the filename
    // of the #include directive.  Clang normally represents both "path" and
    // <path> filenames as a single token, and endLoc refers to the beginning
    // of this token.
    //
    // Things are more complicated if the path comes from a macro.  In that
    // case, a "path" filename is still a single token, but a <path> filename
    // will be several tokens.  In that case, endLoc refers to the beginning of
    // the '>' token.  The '<' and '>' tokens can be arbitrarily positioned.
    // For example, this code successfully includes vector:
    //
    //     #define CONCAT(x, y) x ## y
    //     #define END >
    //     #define QUOTE(x) <x
    //     #include QUOTE(CONCAT(vec,tor))END
    //
    // The approach taken here is to start with the endLoc location and do one
    // of three things:
    // (1) Scan to the end of the quoted token (e.g. find the next '"')
    // (2) Scan to the end of the bracketed token (e.g. find the next '>')
    // (3) Scan backward to the first '<', if it exists.
    //
    // The cfe-commits list has discussed changing InclusionDirective to make
    // finding the filename more feasible.  See
    // http://lists.cs.uiuc.edu/pipermail/cfe-commits/Week-of-Mon-20120917/064618.html
    // This code will probably need to change for the release after Clang 3.1.

    clang::Preprocessor &p = m_context.preprocessor();
    clang::SourceManager &sm = m_context.sourceManager();

    const clang::SourceLocation lastTokenLoc = sm.getSpellingLoc(endLoc);
    const clang::FileID refFileID = lastTokenLoc.isValid() ?
                sm.getFileID(lastTokenLoc) : clang::FileID();
    IndexerFileContext &fileContext = m_context.fileContext(refFileID);
    const clang::SourceLocation lastTokenEndLoc = lastTokenLoc.isValid() ?
                p.getLocForEndOfToken(lastTokenLoc) : clang::SourceLocation();

    Location startIdxLoc = fileContext.location(lastTokenLoc);
    Location endIdxLoc = fileContext.location(lastTokenEndLoc);

    if (isAngled && lastTokenLoc.isValid()) {
        bool invalid = false;
        const char *data = sm.getCharacterData(lastTokenLoc, &invalid);

        if (!invalid && *data == '<') {
            // lastTokenLoc actually points at an angle_string_literal token,
            // but the getLocForEndOfToken method will assume it's looking at a
            // less-than sign and fail to advance to the end of the real token.
            // Work around this by manually scanning for the '>' character.
            unsigned int pos;
            for (pos = 0; true; ++pos) {
                char ch = data[pos];
                if (ch == '>') {
                    pos++;
                    break;
                } else if (ch == '\r' || ch == '\n' || ch == '\0') {
                    break;
                }
            }
            endIdxLoc = startIdxLoc;
            endIdxLoc.column += pos;
        } else if (!invalid && *data == '>') {
            // Heuristic.  Attempt to find a '<' by searching backwards.  We
            // won't always find one -- the '<' could be on a different line,
            // after the '>' character, etc.
            std::pair<clang::FileID, unsigned int> slocPair =
                    sm.getDecomposedLoc(lastTokenLoc);
            bool invalid = false;
            const llvm::MemoryBuffer *buffer =
                    sm.getBuffer(slocPair.first, &invalid);
            const char *bufferStart = buffer->getBufferStart();
            assert(data == bufferStart + slocPair.second);
            unsigned int pos;
            for (pos = slocPair.second; pos > 0; --pos) {
                char ch = bufferStart[pos];
                if (ch == '<') {
                    break;
                } else if (ch == '\n' || ch == '\r') {
                    ++pos;
                    break;
                }
            }
            // Compute a count of characters before the '>', which includes the
            // '<' but not the '>'.
            int charCount = slocPair.second - pos;
            startIdxLoc.column = std::max(1, startIdxLoc.column - charCount);
        }
    }

    return std::make_tuple(&fileContext, startIdxLoc, endIdxLoc);
}

void IndexerPPCallbacks::MacroExpands(
        const clang::Token &macroNameToken,
        const clang::MacroInfo *mi,
        clang::SourceRange range)
{
    assert(mi != NULL);
    recordReference(macroNameToken, RT_Expansion);
}

void IndexerPPCallbacks::MacroDefined(
        const clang::Token &macroNameToken,
        const clang::MacroInfo *mi)
{
    assert(mi != NULL);
    recordReference(macroNameToken, RT_Definition);
}

void IndexerPPCallbacks::MacroUndefined(
        const clang::Token &macroNameToken,
        const clang::MacroInfo *mi)
{
    assert(mi != NULL);
    recordReference(macroNameToken, RT_Undefinition);
}

void IndexerPPCallbacks::Defined(const clang::Token &macroNameToken)
{
    recordReference(macroNameToken, RT_DefinedTest);
}

void IndexerPPCallbacks::recordReference(
        const clang::Token &macroNameToken,
        RefType refType)
{
    llvm::StringRef macroName = macroNameToken.getIdentifierInfo()->getName();
    m_tempSymbolName.clear();
    m_tempSymbolName.push_back('#');
    m_tempSymbolName.append(macroName.data(), macroName.size());
    clang::FileID fileID;
    clang::SourceLocation sloc = m_context.sourceManager().getSpellingLoc(
                macroNameToken.getLocation());
    if (sloc.isValid())
        fileID = m_context.sourceManager().getFileID(sloc);
    IndexerFileContext &fileContext = m_context.fileContext(fileID);
    Location start = fileContext.location(sloc);
    Location end = start;
    end.column += macroName.size();
    indexdb::ID symbolID =
            fileContext.builder().insertSymbol(m_tempSymbolName.c_str());
    fileContext.builder().recordRef(
                symbolID, start, end, fileContext.getRefTypeID(refType));
    fileContext.builder().recordSymbol(
                symbolID, fileContext.getSymbolTypeID(ST_Macro));
    fileContext.builder().recordGlobalSymbol(symbolID);
}

} // namespace indexer
