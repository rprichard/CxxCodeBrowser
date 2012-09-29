#include "IndexBuilder.h"

#include "../libindexdb/IndexDb.h"
#include "Location.h"

namespace indexer {

indexdb::Index *newIndex()
{
    indexdb::Index *index = new indexdb::Index;

    index->addStringTable("Path");
    index->addStringTable("Symbol");
    index->addStringTable("SymbolType");
    index->addStringTable("ReferenceType");

    // A list of all references to a symbol.  A reference is a location and a
    // type (such as Definition, Declaration, Call).
    std::vector<std::string> symbolToReference;
    symbolToReference.push_back("Symbol");
    symbolToReference.push_back("ReferenceType");
    symbolToReference.push_back("Path");
    symbolToReference.push_back(""); // Line (1-based)
    symbolToReference.push_back(""); // StartColumn (1-based)
    symbolToReference.push_back(""); // EndColumn (1-based)
    index->addTable("SymbolToReference", symbolToReference);

    // This table is an index of SymbolToReference by location.
    std::vector<std::string> locationToSymbol;
    locationToSymbol.push_back("Path");
    locationToSymbol.push_back(""); // Line (1-based)
    locationToSymbol.push_back(""); // StartColumn (1-based)
    locationToSymbol.push_back(""); // EndColumn (1-based)
    locationToSymbol.push_back("Symbol");
    index->addTable("LocationToSymbol", locationToSymbol);

    // For every symbol in SymbolToReference, this table provides a type (such
    // as Type, Function, Member, LocalVariable).  (TODO: Work out the exact
    // set of symbol types -- prefer having more symbol types -- the navigator
    // can have a large table mapping symbol tables to syntax colors.  The
    // syntax coloring is already C++-aware.)
    std::vector<std::string> symbol;
    symbol.push_back("Symbol");
    symbol.push_back("SymbolType");
    index->addTable("Symbol", symbol);

    // A list of "global" symbols, mostly useful for the "Go to symbol" dialog.
    // It should exclude parameters and local variables.
    std::vector<std::string> globalSymbolList;
    globalSymbolList.push_back("Symbol");
    index->addTable("GlobalSymbol", globalSymbolList);

    // The information in the IncludeToReference and LocationToInclude tables
    // could be folded into the corresponding tables for symbols (by using a
    // single StringTable for paths and normal symbols).  I wasn't sure if
    // doing that would be a good idea or not.  This folding would be
    // convenient for navigating from an #include directive, and showing
    // references of a path makes sense.  However, the navigator would want to
    // jump to the definition of a path, which makes no sense.  We might also
    // want to display an include graph, and filtering LocationToSymbol for
    // includes would be inefficient.

    // A table from an included file to the places including it.
    std::vector<std::string> includeToReference;
    includeToReference.push_back("Path");   // Included file
    includeToReference.push_back("Path");   // Including file
    includeToReference.push_back("");       // Including file Line (1-based)
    includeToReference.push_back("");       // Including file StartColumn (1-based)
    includeToReference.push_back("");       // Including file EndColumn (1-based)
    index->addTable("IncludeToReference", includeToReference);

    std::vector<std::string> locationToInclude;
    locationToInclude.push_back("Path");     // File
    locationToInclude.push_back("");         // Line (1-based)
    locationToInclude.push_back("");         // StartColumn (1-based)
    locationToInclude.push_back("");         // EndColumn (1-based)
    locationToInclude.push_back("Path");     // Included file
    index->addTable("LocationToInclude", locationToInclude);

    return index;
}

IndexBuilder::IndexBuilder(indexdb::Index &index) : m_index(index)
{
    // String tables.
    m_pathStringTable           = m_index.stringTable("Path");
    m_symbolStringTable         = m_index.stringTable("Symbol");
    m_symbolTypeStringTable     = m_index.stringTable("SymbolType");
    m_referenceTypeStringTable  = m_index.stringTable("ReferenceType");

    // Tables.
    m_symbolToReferenceTable    = m_index.table("SymbolToReference");
    m_locationToSymbolTable     = m_index.table("LocationToSymbol");
    m_symbolTable               = m_index.table("Symbol");
    m_globalSymbolTable         = m_index.table("GlobalSymbol");
    m_includeToReferenceTable   = m_index.table("IncludeToReference");
    m_locationToInclude         = m_index.table("LocationToInclude");
}

void IndexBuilder::recordRef(
        indexdb::ID symbolID,
        const Location &start,
        const Location &end,
        indexdb::ID refTypeID)
{
    int startColumn = start.column;
    int endColumn = start.column;
    if (end.fileID == start.fileID && end.line == start.line)
        endColumn = std::max(endColumn, end.column);

    {
        indexdb::Row symbolToReference(6);
        symbolToReference[0] = symbolID;
        symbolToReference[1] = refTypeID;
        symbolToReference[2] = start.fileID;
        symbolToReference[3] = start.line;
        symbolToReference[4] = startColumn;
        symbolToReference[5] = endColumn;
        m_symbolToReferenceTable->add(symbolToReference);
    }

    {
        indexdb::Row locationToSymbol(5);
        locationToSymbol[0] = start.fileID;
        locationToSymbol[1] = start.line;
        locationToSymbol[2] = startColumn;
        locationToSymbol[3] = endColumn;
        locationToSymbol[4] = symbolID;
        m_locationToSymbolTable->add(locationToSymbol);
    }
}

void IndexBuilder::recordSymbol(
        indexdb::ID symbolID,
        indexdb::ID symbolTypeID)
{
    indexdb::Row symbol(2);
    symbol[0] = symbolID;
    symbol[1] = symbolTypeID;
    m_symbolTable->add(symbol);
}

} // namespace indexer
