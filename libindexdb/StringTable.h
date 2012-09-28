#ifndef INDEXDB_STRINGTABLE_H
#define INDEXDB_STRINGTABLE_H

#include <cassert>
#include <stdint.h>
#include <utility>

#include "Buffer.h"
#include "Util.h"

#define STRING_TABLE_STATS 0

namespace indexdb {

class Reader;
class Writer;
typedef uint32_t ID;
const ID kInvalidID = static_cast<ID>(-1);

class StringTable
{
private:
    struct TableNode {
        uint32_t offset;
        uint32_t size;
        uint32_t hash;
        ID indexNext;
    };

private:
    Buffer m_data;
    Buffer m_table;
    Buffer m_index;
    bool m_nullTerminateStrings;
#if STRING_TABLE_STATS
    mutable uint64_t m_accesses;
    mutable uint64_t m_probes;
#endif

    char *dataPtr()             { return static_cast<char*>(m_data.data()); }
    const char *dataPtr() const { return static_cast<const char*>(m_data.data()); }

    TableNode *tablePtr()             { return static_cast<TableNode*>(m_table.data()); }
    const TableNode *tablePtr() const { return static_cast<const TableNode*>(m_table.data()); }

    ID *indexPtr()             { return static_cast<ID*>(m_index.data()); }
    const ID *indexPtr() const { return static_cast<const ID*>(m_index.data()); }

    uint32_t indexSize() const { return m_index.size() / sizeof(ID); }
    void resizeHashTable(uint32_t newIndexSize);
    inline ID lookup(const char *data, uint32_t dataSize, uint32_t hash) const;
    ID insert(const char *data, uint32_t dataSize, uint32_t hash);

public:
    explicit StringTable(bool nullTerminateStrings=true);
    StringTable(StringTable &&other);
    explicit StringTable(Reader &reader);
    void write(Writer &writer);

    ID id(const char *string) const;
    ID insert(const char *string);
    ID insert(const char *string, uint32_t size);
    void dumpStats() const;

    const char *item(ID id) const {
        assert(id < size());
        return dataPtr() + LEToHost32(tablePtr()[id].offset);
    }

    uint32_t itemSize(ID id) const {
        assert(id < size());
        return LEToHost32(tablePtr()[id].size);
    }

    uint32_t itemHash(ID id) const {
        assert(id < size());
        return LEToHost32(tablePtr()[id].hash);
    }

    uint32_t size() const { return m_table.size() / sizeof(TableNode); }
    uint32_t contentByteSize() const { return m_data.size(); }
    Buffer pillageContent() { return std::move(m_data); }

    friend class Index;
};

} // namespace indexdb

#endif // INDEXDB_STRINGTABLE_H
