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
    auto &builder = m_context.indexBuilder();
    m_refTypeExpansion = builder.insertRefType("Expansion");
    m_refTypeDefinition = builder.insertRefType("Definition");
    m_refTypeDefinedTest = builder.insertRefType("Defined-Test");
}

void IndexerPPCallbacks::MacroExpands(
        const clang::Token &macroNameToken,
        const clang::MacroInfo *mi,
        clang::SourceRange range)
{
    recordReference(macroNameToken, m_refTypeExpansion);
}

void IndexerPPCallbacks::MacroDefined(
        const clang::Token &macroNameToken,
        const clang::MacroInfo *mi)
{
    recordReference(macroNameToken, m_refTypeDefinition);
}

void IndexerPPCallbacks::Defined(const clang::Token &macroNameToken)
{
    recordReference(macroNameToken, m_refTypeDefinedTest);
}

void IndexerPPCallbacks::recordReference(
        const clang::Token &macroNameToken,
        indexdb::ID refTypeID)
{
    llvm::StringRef macroName = macroNameToken.getIdentifierInfo()->getName();
    m_tempSymbolName.clear();
    m_tempSymbolName.push_back('@');
    m_tempSymbolName.append(macroName.data(), macroName.size());
    Location start = m_context.locationConverter().convert(
                macroNameToken.getLocation());
    Location end = start;
    end.column += macroName.size();
    m_context.indexBuilder().recordRef(
                m_context.indexBuilder().insertSymbol(
                    m_tempSymbolName.c_str()),
                start, end, refTypeID);
}

} // namespace indexer
