#include "IndexerPPCallbacks.h"

#include <clang/Basic/IdentifierTable.h>
#include <iostream>

#include "IndexBuilder.h"
#include "IndexerContext.h"
#include "Location.h"

namespace indexer {

void IndexerPPCallbacks::MacroExpands(
        const clang::Token &macroNameToken,
        const clang::MacroInfo *mi,
        clang::SourceRange range)
{
    recordReference(macroNameToken, "Expansion");
}

void IndexerPPCallbacks::MacroDefined(
        const clang::Token &macroNameToken,
        const clang::MacroInfo *mi)
{
    recordReference(macroNameToken, "Definition");
}

void IndexerPPCallbacks::Defined(const clang::Token &macroNameToken)
{
    recordReference(macroNameToken, "Defined-Test");
}

void IndexerPPCallbacks::recordReference(
        const clang::Token &macroNameToken,
        const char *kind)
{
    std::string symbolName = "@";
    llvm::StringRef macroName = macroNameToken.getIdentifierInfo()->getName();
    symbolName.append(macroName.data(), macroName.size());
    Location start = m_context.locationConverter().convert(
                macroNameToken.getLocation());
    Location end = start;
    end.column += macroName.size();
    m_context.indexBuilder().recordRef(symbolName.c_str(), start, end, kind);
}

} // namespace indexer
