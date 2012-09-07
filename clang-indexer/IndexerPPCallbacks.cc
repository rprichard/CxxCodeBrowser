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
    std::string usr = "c:macro@";
    llvm::StringRef macroName = macroNameToken.getIdentifierInfo()->getName();
    usr.append(macroName.data(), macroName.size());
    Location loc = m_context.locationConverter().convert(
                macroNameToken.getLocation());
    m_context.indexBuilder().recordRef(usr.c_str(), loc, kind);
}

} // namespace indexer
