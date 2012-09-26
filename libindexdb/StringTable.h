#ifndef INDEXDB_STRINGTABLE_H
#define INDEXDB_STRINGTABLE_H

#include <stdint.h>
#include <cassert>
#include "Buffer.h"

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
    StringTable();
    StringTable(StringTable &&other);
    StringTable(Reader &reader);
    void write(Writer &writer);

    ID id(const char *string) const;
    ID insert(const char *string);

    const char *item(ID id) const {
        assert(id < size());
        return dataPtr() + tablePtr()[id].offset;
    }

    uint32_t itemSize(ID id) const {
        assert(id < size());
        return tablePtr()[id].size;
    }

    uint32_t itemHash(ID id) const {
        assert(id < size());
        return tablePtr()[id].hash;
    }

    uint32_t size() const { return m_table.size() / sizeof(TableNode); }

    uint32_t contentSize() const { return m_data.size(); }

    friend class Index;
};

} // namespace indexdb

#endif // INDEXDB_STRINGTABLE_H
