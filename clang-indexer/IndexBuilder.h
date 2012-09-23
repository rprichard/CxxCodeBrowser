#ifndef INDEXER_INDEXBUILDER_H
#define INDEXER_INDEXBUILDER_H

#include "../libindexdb/IndexDb.h"

namespace indexer {

class Location;

// Creates a new indexdb::Index with empty tables.
indexdb::Index *newIndex();

class IndexBuilder
{
public:
    IndexBuilder(indexdb::Index &index);
    void recordRef(
            const char *symbolName,
            const Location &start,
            const Location &end,
            const char *refType);
    void recordSymbol(
            const char *symbolName,
            const char *symbolType);
    indexdb::ID insertPath(const char *path) { return m_pathStringTable->insert(path); }
    const char *lookupPath(indexdb::ID pathID) { return m_pathStringTable->item(pathID); }

private:
    // The IndexBuilder instance does not own m_index.
    indexdb::Index &m_index;

    indexdb::StringTable *m_pathStringTable;
    indexdb::StringTable *m_symbolStringTable;
    indexdb::StringTable *m_symbolTypeStringTable;
    indexdb::StringTable *m_referenceTypeStringTable;
    indexdb::Table *m_symbolToReferenceTable;
    indexdb::Table *m_locationToSymbolTable;
    indexdb::Table *m_symbolTable;
    indexdb::Table *m_globalSymbolTable;
    indexdb::Table *m_includeToReferenceTable;
    indexdb::Table *m_locationToInclude;
};

} // namespace indexer

#endif // INDEXER_INDEXBUILDER_H
