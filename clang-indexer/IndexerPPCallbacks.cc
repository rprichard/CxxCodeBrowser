#include "IndexerPPCallbacks.h"

#include <clang/Basic/IdentifierTable.h>
#include <iostream>

#include "IndexBuilder.h"
#include "Location.h"

namespace indexer {

void IndexerPPCallbacks::MacroExpands(
        const clang::Token &macroNameToken,
        const clang::MacroInfo *mi,
        clang::SourceRange range)
{
    recordReference(macroNameToken, /*range.getBegin(),*/ "Expansion");
}

void IndexerPPCallbacks::MacroDefined(
        const clang::Token &macroNameToken,
        const clang::MacroInfo *mi)
{
    recordReference(macroNameToken, /*mi->getDefinitionLoc(),*/ "Definition");
}

void IndexerPPCallbacks::Defined(const clang::Token &macroNameToken)
{
    recordReference(macroNameToken, /*macroNameToken.getLocation(),*/ "Defined-Test");
}

void IndexerPPCallbacks::recordReference(
        const clang::Token &macroNameToken,
        //clang::SourceLocation location,
        const char *kind)
{
    std::string usr = "c:macro@";
    llvm::StringRef macroName = macroNameToken.getIdentifierInfo()->getName();
    usr.append(macroName.data(), macroName.size());
    Location loc = convertLocation(m_pSM, macroNameToken.getLocation() /*, location*/);
    m_builder.recordRef(usr.c_str(), loc, kind);
}

} // namespace indexer
