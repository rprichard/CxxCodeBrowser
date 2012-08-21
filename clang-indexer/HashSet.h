#ifndef INDEXDB_HASHSET_H
#define INDEXDB_HASHSET_H

#include <stdint.h>
#include "Buffer.h"

namespace indexdb {

class Reader;
class Writer;
typedef uint32_t ID;
const ID kInvalidID = static_cast<ID>(-1);

template <typename DataType>
class HashSet
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

    DataType *dataPtr() { return static_cast<DataType*>(m_data.data()); }
    TableNode *tablePtr() { return static_cast<TableNode*>(m_table.data()); }
    ID *indexPtr() { return static_cast<ID*>(m_index.data()); }
    uint32_t indexSize() { return m_index.size() / sizeof(ID); }
    void resizeHashTable(uint32_t newIndexSize);

public:
    HashSet();
    HashSet(Reader &reader);
    void write(Writer &writer);

    ID id(const DataType *data, uint32_t dataSize, uint32_t hash);

    std::pair<const DataType*, uint32_t> data(ID id) {
        TableNode *n = &tablePtr()[id];
        return std::make_pair(&dataPtr()[n->offset], n->size);
    }

    uint32_t hash(ID id) {
        return tablePtr()[id].hash;
    }

    uint32_t size() { return m_table.size() / sizeof(TableNode); }
};

} // namespace indexdb

#include "HashSet-inl.h"

#endif // INDEXDB_HASHSET_H
