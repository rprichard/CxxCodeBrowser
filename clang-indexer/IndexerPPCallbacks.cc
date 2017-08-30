#include "IndexerPPCallbacks.h"

#include <clang/Basic/IdentifierTable.h>
#include <clang/Lex/Preprocessor.h>
#include <iostream>

#include "IndexBuilder.h"
#include "IndexerContext.h"
#include "Location.h"
#include "Util.h"

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
        clang::CharSourceRange filenameRange,
        const clang::FileEntry *file,
        llvm::StringRef searchPath,
        llvm::StringRef relativePath,
        const clang::Module *imported)
{
    if (file == NULL) {
        // The file can be NULL, in which case there is nothing for the indexer
        // to record.  (For example, the target of an #include might not be
        // found.)
        return;
    }

    // Get the location of the #include filename.
    auto range = getIncludeFilenameLoc(filenameRange);
    IndexerFileContext &fileContext = *std::get<0>(range);

    // Get the path ID of the included file.
    indexdb::ID symbolID;
    auto it = m_includePathMap.find(file);
    if (it == m_includePathMap.end()) {
        std::string symbol = "@";
        llvm::StringRef fname = file->getName();
        char *path = portableRealPath(fname.data());
        if (path != NULL) {
            symbol += path;
            free(path);
        }
        if (symbol.size() == 1)
            symbol += "<blank>";
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
    fileContext.builder().recordSymbol(
                symbolID,
                fileContext.getSymbolTypeID(ST_Path));
}

std::tuple<IndexerFileContext*, Location, Location>
IndexerPPCallbacks::getIncludeFilenameLoc(
        clang::CharSourceRange filenameRange)
{
    clang::SourceManager &sm = m_context.sourceManager();
    const clang::SourceLocation startLoc =
            sm.getSpellingLoc(filenameRange.getBegin());
    const clang::SourceLocation endLoc =
            sm.getSpellingLoc(filenameRange.getEnd());
    const clang::FileID refFileID = startLoc.isValid() ?
                sm.getFileID(startLoc) : clang::FileID();
    IndexerFileContext &fileContext = m_context.fileContext(refFileID);
    return std::make_tuple(&fileContext,
                           fileContext.location(startLoc),
                           fileContext.location(endLoc));
}

void IndexerPPCallbacks::MacroExpands(
        const clang::Token &macroNameToken,
        const clang::MacroDefinition &md,
        clang::SourceRange range,
        const clang::MacroArgs *args)
{
    recordReference(macroNameToken, RT_Expansion);
}

void IndexerPPCallbacks::MacroDefined(
        const clang::Token &macroNameToken,
        const clang::MacroDirective *md)
{
    recordReference(macroNameToken, RT_Definition);
}

void IndexerPPCallbacks::MacroUndefined(
        const clang::Token &macroNameToken,
        const clang::MacroDefinition &md)
{
    recordReference(macroNameToken, RT_Undefinition);
}

void IndexerPPCallbacks::Defined(
        const clang::Token &macroNameToken,
        const clang::MacroDefinition &md,
        clang::SourceRange range)
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
