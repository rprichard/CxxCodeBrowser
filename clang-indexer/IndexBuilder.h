#ifndef INDEXER_INDEXBUILDER_H
#define INDEXER_INDEXBUILDER_H

#include "../libindexdb/IndexDb.h"

namespace indexer {

class Location;

// This class creates indexdb databases with the appropriate scheme and
// provides methods to populate the database.  It should be mostly independent
// of the indexed language.
class IndexBuilder
{
public:
    IndexBuilder(indexdb::Index &index, bool createIndexTables=true);
    void populateIndexTables();

    void recordRef(
            indexdb::ID symbolID,
            const Location &start,
            const Location &end,
            indexdb::ID refTypeID);
    void recordSymbol(
            indexdb::ID symbolID,
            indexdb::ID symbolTypeID);

    indexdb::ID insertSymbol(const char *symbol) { return m_symbolStringTable->insert(symbol); }
    indexdb::ID insertRefType(const char *refType) { return m_refTypeStringTable->insert(refType); }
    indexdb::ID insertSymbolType(const char *symbolType) { return m_symbolTypeStringTable->insert(symbolType); }
    const char *lookupSymbol(indexdb::ID symbolID) { return m_symbolStringTable->item(symbolID); }

private:
    // The IndexBuilder instance does not own m_index.
    indexdb::Index &m_index;

    indexdb::StringTable *m_symbolStringTable;
    indexdb::StringTable *m_symbolTypeStringTable;
    indexdb::StringTable *m_refTypeStringTable;
    indexdb::Table *m_refTable;
    indexdb::Table *m_refIndexTable;
    indexdb::Table *m_symbolTable;
    indexdb::Table *m_symbolTypeIndexTable;
    indexdb::Table *m_globalSymbolTable;
};

} // namespace indexer

#endif // INDEXER_INDEXBUILDER_H
