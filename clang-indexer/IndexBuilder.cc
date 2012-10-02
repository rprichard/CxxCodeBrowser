#include "IndexBuilder.h"

#include "../libindexdb/IndexDb.h"
#include "Location.h"

namespace indexer {

// Constructs an IndexBuilder that populates the given Index.  It does not
// take ownership of the Index object.  If createLocationTables is false, the
// IndexBuilder does not build or populate the LocationToSymbol and
// LocationToInclude tables.  These tables can be generated later by inverting
// the SymbolToReference and IncludeToReference tables (typically after merging
// indices).
IndexBuilder::IndexBuilder(indexdb::Index &index, bool createLocationTables) :
    m_index(index)
{
    // String tables.
    m_pathStringTable =             index.addStringTable("Path");
    m_symbolStringTable =           index.addStringTable("Symbol");
    m_symbolTypeStringTable =       index.addStringTable("SymbolType");
    m_referenceTypeStringTable =    index.addStringTable("ReferenceType");

    // A list of all references to a symbol.  A reference is a location and a
    // type (such as Definition, Declaration, Call).
    std::vector<std::string> symbolToReference;
    symbolToReference.push_back("Symbol");
    symbolToReference.push_back("ReferenceType");
    symbolToReference.push_back("Path");
    symbolToReference.push_back(""); // Line (1-based)
    symbolToReference.push_back(""); // StartColumn (1-based)
    symbolToReference.push_back(""); // EndColumn (1-based)
    m_symbolToReferenceTable = index.addTable("SymbolToReference", symbolToReference);

    if (createLocationTables) {
        // This table is an index of SymbolToReference by location.
        std::vector<std::string> locationToSymbol;
        locationToSymbol.push_back("Path");
        locationToSymbol.push_back(""); // Line (1-based)
        locationToSymbol.push_back(""); // StartColumn (1-based)
        locationToSymbol.push_back(""); // EndColumn (1-based)
        locationToSymbol.push_back("Symbol");
        m_locationToSymbolTable = m_index.addTable("LocationToSymbol", locationToSymbol);
    } else {
        m_locationToSymbolTable = NULL;
    }

    // For every symbol in SymbolToReference, this table provides a type (such
    // as Type, Function, Member, LocalVariable).  (TODO: Work out the exact
    // set of symbol types -- prefer having more symbol types -- the navigator
    // can have a large table mapping symbol tables to syntax colors.  The
    // syntax coloring is already C++-aware.)
    std::vector<std::string> symbol;
    symbol.push_back("Symbol");
    symbol.push_back("SymbolType");
    m_symbolTable = index.addTable("Symbol", symbol);

    // A list of "global" symbols, mostly useful for the "Go to symbol" dialog.
    // It should exclude parameters and local variables.
    std::vector<std::string> globalSymbolList;
    globalSymbolList.push_back("Symbol");
    m_globalSymbolTable = index.addTable("GlobalSymbol", globalSymbolList);

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
    m_includeToReferenceTable = index.addTable("IncludeToReference", includeToReference);

    if (createLocationTables) {
        std::vector<std::string> locationToInclude;
        locationToInclude.push_back("Path");     // File
        locationToInclude.push_back("");         // Line (1-based)
        locationToInclude.push_back("");         // StartColumn (1-based)
        locationToInclude.push_back("");         // EndColumn (1-based)
        locationToInclude.push_back("Path");     // Included file
        m_locationToIncludeTable = m_index.addTable("LocationToInclude", locationToInclude);
    } else {
        m_locationToIncludeTable = NULL;
    }
}

void IndexBuilder::populateLocationTables()
{
    assert(m_symbolToReferenceTable->isReadOnly());
    assert(m_includeToReferenceTable->isReadOnly());
    assert(!m_locationToSymbolTable->isReadOnly());
    assert(!m_locationToIncludeTable->isReadOnly());

    {
        indexdb::Row srcRow(m_symbolToReferenceTable->columnCount());
        indexdb::Row destRow(m_locationToSymbolTable->columnCount());
        for (auto it = m_symbolToReferenceTable->begin(),
                itEnd = m_symbolToReferenceTable->end();
                it != itEnd; ++it) {
            it.value(srcRow);
            destRow[0] = srcRow[2];
            destRow[1] = srcRow[3];
            destRow[2] = srcRow[4];
            destRow[3] = srcRow[5];
            destRow[4] = srcRow[0];
            m_locationToSymbolTable->add(destRow);
        }
    }

    {
        indexdb::Row srcRow(m_includeToReferenceTable->columnCount());
        indexdb::Row destRow(m_locationToIncludeTable->columnCount());
        for (auto it = m_includeToReferenceTable->begin(),
                itEnd = m_includeToReferenceTable->end();
                it != itEnd; ++it) {
            it.value(srcRow);
            destRow[0] = srcRow[1];
            destRow[1] = srcRow[2];
            destRow[2] = srcRow[3];
            destRow[3] = srcRow[4];
            destRow[4] = srcRow[0];
            m_locationToSymbolTable->add(destRow);
        }
    }
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

    if (m_locationToSymbolTable != NULL) {
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
