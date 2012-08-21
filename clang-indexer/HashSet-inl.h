#include "HashSet.h"
#include "Buffer.h"
#include "FileIo.h"
#include <cstring>
#include <cassert>

namespace indexdb {

template <typename DataType>
HashSet<DataType>::HashSet() :
    m_index(1024, 0xFF)
{
}

template <typename DataType>
HashSet<DataType>::HashSet(Reader &reader)
{
    m_data = reader.readBuffer();
    m_table = reader.readBuffer();
    m_index = reader.readBuffer();
}

template <typename DataType>
void HashSet<DataType>::write(Writer &writer)
{
    writer.writeBuffer(m_data);
    writer.writeBuffer(m_table);
    writer.writeBuffer(m_index);
}

template <typename DataType>
ID HashSet<DataType>::id(const DataType *data, uint32_t dataSize, uint32_t hash)
{
    uint32_t index = hash % indexSize();
    for (ID tableIndex = indexPtr()[index];
            tableIndex != kInvalidID;
            tableIndex = tablePtr()[tableIndex].indexNext) {
        TableNode *n = &tablePtr()[tableIndex];
        if (n->hash == hash && n->size == dataSize &&
                memcmp(dataPtr() + n->offset, data, dataSize * sizeof(DataType)) == 0) {
            return tableIndex;
        }
    }

    if (this->size() * 2 > indexSize()) {
        resizeHashTable(this->indexSize() * 2);
        index = hash % indexSize();
    }

    ID newNodeID = size();
    TableNode newNode;
    newNode.offset = m_data.size() / sizeof(DataType);
    newNode.size = dataSize;
    newNode.hash = hash;
    newNode.indexNext = indexPtr()[index];
    indexPtr()[index] = newNodeID;
    m_data.append(data, dataSize * sizeof(DataType));
    m_table.append(&newNode, sizeof(newNode));

    return newNodeID;
}

template <typename DataType>
void HashSet<DataType>::resizeHashTable(uint32_t newIndexSize)
{
    m_index = Buffer(newIndexSize * sizeof(ID), 0xFF);
    for (ID i = 0; i < size(); ++i) {
        tablePtr()[i].indexNext = kInvalidID;
    }
    for (ID i = 0; i < size(); ++i) {
        TableNode *n = &tablePtr()[i];
        uint32_t indexIndex = n->hash % newIndexSize;
        n->indexNext = indexPtr()[indexIndex];
        indexPtr()[indexIndex] = i;
    }
}

} // namespace indexdb
