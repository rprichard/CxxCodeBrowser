#include "IndexerPPCallbacks.h"

#include <clang/Basic/IdentifierTable.h>
#include <iostream>

#include "IndexBuilder.h"
#include "IndexerContext.h"
#include "Location.h"

namespace indexer {

IndexerPPCallbacks::IndexerPPCallbacks(IndexerContext &context) :
    m_context(context)
{
}

void IndexerPPCallbacks::MacroExpands(
        const clang::Token &macroNameToken,
        const clang::MacroInfo *mi,
        clang::SourceRange range)
{
    recordReference(macroNameToken, RT_Expansion);
}

void IndexerPPCallbacks::MacroDefined(
        const clang::Token &macroNameToken,
        const clang::MacroInfo *mi)
{
    recordReference(macroNameToken, RT_Definition);
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
    m_tempSymbolName.push_back('@');
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
    fileContext.builder().recordRef(
                fileContext.builder().insertSymbol(
                    m_tempSymbolName.c_str()),
                start, end, fileContext.getRefTypeID(refType));
}

} // namespace indexer
