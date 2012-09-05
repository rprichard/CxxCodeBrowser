#include "IndexerPPCallbacks.h"

#include <clang/Basic/IdentifierTable.h>
#include <iostream>

#include "IndexBuilder.h"
#include "Location.h"

namespace indexer {

void IndexerPPCallbacks::MacroExpands(const clang::Token &macroNameTok,
                                 const clang::MacroInfo *mi,
                                 clang::SourceRange range)
{
    Location loc = convertLocation(m_pSM, range.getBegin());
    std::cerr << loc.toString() << ": expand "
              << macroNameTok.getIdentifierInfo()->getName().str() << std::endl;
}

void IndexerPPCallbacks::MacroDefined(const clang::Token &macroNameTok,
                                 const clang::MacroInfo *MI)
{
    Location loc = convertLocation(m_pSM, MI->getDefinitionLoc());
    std::cerr << loc.toString() << ": #define "
              << macroNameTok.getIdentifierInfo()->getName().str() << std::endl;
}

void IndexerPPCallbacks::Defined(const clang::Token &macroNameTok)
{
    Location loc = convertLocation(m_pSM, macroNameTok.getLocation());
    std::cerr << loc.toString() << ": defined(): "
              << macroNameTok.getIdentifierInfo()->getName().str() << std::endl;
}

} // namespace indexer
