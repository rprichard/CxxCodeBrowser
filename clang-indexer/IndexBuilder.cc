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
IndexBuilder::IndexBuilder(indexdb::Index &index, bool createIndexTables) :
    m_index(index)
{
    // String tables.
    m_symbolStringTable     = index.addStringTable("Symbol");
    m_symbolTypeStringTable = index.addStringTable("SymbolType");
    m_refTypeStringTable    = index.addStringTable("ReferenceType");

    // Note: A "column" is really a byte offset into the line's content.
    // Assuming the file is encoded with UTF-8, a tab character occupies just
    // one column, but a Unicode code point may occupy several columns.  An
    // "end" column refers to the position just past the reference, not to the
    // last character of it.

    // A list of all references to a symbol.  A reference is a location and a
    // type (such as Definition, Declaration, Call).
    std::vector<std::string> refColumns;
    refColumns.push_back("Symbol");         // Path symbol              // (LOC)
    refColumns.push_back("");               // Line (1-based)           // (LOC)
    refColumns.push_back("");               // StartColumn (1-based)    // (LOC)
    refColumns.push_back("");               // EndColumn (1-based)      // (LOC)
    refColumns.push_back("Symbol");         // Symbol referenced        // (REF)
    refColumns.push_back("ReferenceType");  // Type of reference        // (REF)
    m_refTable = index.addTable("Reference", refColumns);

    if (createIndexTables) {
        // This table is an inverted version of the Reference table.
        std::vector<std::string> refIndexColumns;
        refIndexColumns.push_back("Symbol");        // Symbol referenced        (REF)
        refIndexColumns.push_back("ReferenceType"); // Type of reference        (REF)
        refIndexColumns.push_back("Symbol");        // Path symbol              (LOC)
        refIndexColumns.push_back("");              // Line (1-based)           (LOC)
        refIndexColumns.push_back("");              // StartColumn (1-based)    (LOC)
        refIndexColumns.push_back("");              // EndColumn (1-based)      (LOC)
        m_refIndexTable = m_index.addTable("ReferenceIndex", refIndexColumns);
    } else {
        m_refIndexTable = NULL;
    }

    // For every symbol in SymbolToReference, this table provides a type (such
    // as Type, Function, Member, LocalVariable).  (TODO: Work out the exact
    // set of symbol types -- prefer having more symbol types -- the navigator
    // can have a large table mapping symbol tables to syntax colors.  The
    // syntax coloring is already C++-aware.)
    std::vector<std::string> symbolColumns;
    symbolColumns.push_back("Symbol");
    symbolColumns.push_back("SymbolType");
    m_symbolTable = index.addTable("Symbol", symbolColumns);

    // An index of the Symbol table (SymbolType -> Symbol)
    if (createIndexTables) {
        std::vector<std::string> symbolTypeIndexColumns;
        symbolTypeIndexColumns.push_back("SymbolType");
        symbolTypeIndexColumns.push_back("Symbol");
        m_symbolTypeIndexTable = index.addTable("SymbolTypeIndex", symbolTypeIndexColumns);
    } else {
        m_symbolTypeIndexTable = NULL;
    }

    // A list of "global" symbols, mostly useful for the "Go to symbol" dialog.
    // It should exclude parameters and local variables.
    std::vector<std::string> globalSymbolList;
    globalSymbolList.push_back("Symbol");
    m_globalSymbolTable = index.addTable("GlobalSymbol", globalSymbolList);
}

// Populate the ReferenceIndex table by inverting the Reference table.
// Populate the SymbolTypeIndex table by inverting the Symbol table.
void IndexBuilder::populateIndexTables()
{
    assert(m_refTable->isReadOnly());
    assert(!m_refIndexTable->isReadOnly());

    {
        indexdb::Row srcRow(m_refTable->columnCount());
        indexdb::Row destRow(m_refIndexTable->columnCount());
        for (auto it = m_refTable->begin(),
                itEnd = m_refTable->end();
                it != itEnd; ++it) {
            it.value(srcRow);
            destRow[0] = srcRow[4];
            destRow[1] = srcRow[5];
            destRow[2] = srcRow[0];
            destRow[3] = srcRow[1];
            destRow[4] = srcRow[2];
            destRow[5] = srcRow[3];
            m_refIndexTable->add(destRow);
        }
    }

    {
        indexdb::Row srcRow(m_symbolTable->columnCount());
        indexdb::Row destRow(m_symbolTypeIndexTable->columnCount());
        for (auto it = m_symbolTable->begin(),
                itEnd = m_symbolTable->end();
                it != itEnd; ++it) {
            it.value(srcRow);
            destRow[0] = srcRow[1];
            destRow[1] = srcRow[0];
            m_symbolTypeIndexTable->add(destRow);
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
        indexdb::Row row(6);
        row[0] = start.fileID;
        row[1] = start.line;
        row[2] = startColumn;
        row[3] = endColumn;
        row[4] = symbolID;
        row[5] = refTypeID;
        m_refTable->add(row);
    }

    if (m_refIndexTable != NULL) {
        // XXX: This code is not currently being tested -- all refs are
        // recorded before the ReferenceIndex table is created.
        indexdb::Row row(6);
        row[0] = symbolID;
        row[1] = refTypeID;
        row[2] = start.fileID;
        row[3] = start.line;
        row[4] = startColumn;
        row[5] = endColumn;
        m_refIndexTable->add(row);
    }
}

void IndexBuilder::recordSymbol(
        indexdb::ID symbolID,
        indexdb::ID symbolTypeID)
{
    indexdb::Row row(2);
    row[0] = symbolID;
    row[1] = symbolTypeID;
    m_symbolTable->add(row);
}

void IndexBuilder::recordGlobalSymbol(indexdb::ID symbolID)
{
    indexdb::Row row(1);
    row[0] = symbolID;
    m_globalSymbolTable->add(row);
}

} // namespace indexer
