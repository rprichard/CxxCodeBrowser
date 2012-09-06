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
    void recordRef(const char *usr, const Location &loc, const char *kind);

    indexdb::ID insertPath(const char *path) { return m_pathStringTable->insert(path); }
    const char *lookupPath(indexdb::ID pathID) { return m_pathStringTable->item(pathID); }

private:
    // The IndexBuilder instance does not own m_index.
    indexdb::Index &m_index;

    indexdb::StringTable *m_pathStringTable;
    indexdb::StringTable *m_kindStringTable;
    indexdb::StringTable *m_usrStringTable;
    indexdb::Table *m_refTable;
    indexdb::Table *m_locTable;
};

} // namespace indexer

#endif // INDEXER_INDEXBUILDER_H
