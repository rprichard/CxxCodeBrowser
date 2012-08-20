#include "HashSet.h"
#include "Buffer.h"
#include "FileIo.h"
#include <cstring>
#include <cassert>

namespace indexdb {

struct HashSetHeader {
    uint32_t dataBufferOffset;
    uint32_t dataBufferSize;
    uint32_t tableBufferOffset;
    uint32_t tableBufferSize;
    uint32_t indexBufferOffset;
    uint32_t indexBufferSize;
};

template <typename DataType>
HashSet<DataType>::HashSet() :
    m_index(1024, 0xFF)
{
}

template <typename DataType>
HashSet<DataType>::HashSet(int fd, uint32_t offset) {
    Buffer b = Buffer::fromFile(fd, offset, sizeof(HashSetHeader));
    HashSetHeader *h = static_cast<HashSetHeader*>(b.data());
    m_data = Buffer::fromFile(fd, h->dataBufferOffset, h->dataBufferSize);
    m_table = Buffer::fromFile(fd, h->tableBufferOffset, h->tableBufferSize);
    m_index = Buffer::fromFile(fd, h->indexBufferOffset, h->indexBufferSize);
}

template <typename DataType>
void HashSet<DataType>::write(int fd)
{
    assert(tell(fd) == roundToPage(tell(fd)));

    HashSetHeader h;
    h.dataBufferOffset = tell(fd) + kPageSize;
    h.dataBufferSize = m_data.size();
    h.tableBufferOffset = roundToPage(h.dataBufferOffset + h.dataBufferSize);
    h.tableBufferSize = m_table.size();
    h.indexBufferOffset = roundToPage(h.tableBufferOffset + h.tableBufferSize);
    h.indexBufferSize = m_index.size();

    ssize_t result = ::write(fd, &h, sizeof(h));
    assert(result == sizeof(h));
    padFile(fd);

    m_data.write(fd);
    m_table.write(fd);
    m_index.write(fd);
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
