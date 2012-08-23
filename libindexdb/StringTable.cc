#include "StringTable.h"

#include <cassert>
#include <cstring>

#include "Buffer.h"
#include "FileIo.h"
#include "MurmurHash3.h"

namespace indexdb {

StringTable::StringTable() :
    m_index(1024, 0xFF)
{
}

StringTable::StringTable(StringTable &&other) :
    m_data(std::move(other.m_data)),
    m_table(std::move(other.m_table)),
    m_index(std::move(other.m_index))
{
}

StringTable::StringTable(Reader &reader)
{
    m_data = reader.readBuffer();
    m_table = reader.readBuffer();
    m_index = reader.readBuffer();
}

void StringTable::write(Writer &writer)
{
    writer.writeBuffer(m_data);
    writer.writeBuffer(m_table);
    writer.writeBuffer(m_index);
}

inline ID StringTable::lookup(
        const char *data,
        uint32_t dataSize,
        uint32_t hash) const
{
    uint32_t index = hash % indexSize();
    for (ID tableIndex = indexPtr()[index];
            tableIndex != kInvalidID;
            tableIndex = tablePtr()[tableIndex].indexNext) {
        const TableNode *n = &tablePtr()[tableIndex];
        if (n->hash == hash && n->size == dataSize &&
                memcmp(dataPtr() + n->offset, data, dataSize) == 0) {
            return tableIndex;
        }
    }
    return kInvalidID;
}

ID StringTable::insert(const char *data, uint32_t dataSize, uint32_t hash)
{
    // If the string table was loaded from a file, then the buffers will be
    // mapped as read-only and writing to them will segfault.  We could make
    // writable copies of the buffers, but there's currently no legitimate
    // reason to modify a mapped string table.
    assert(!m_data.isMapped());

    ID result = lookup(data, dataSize, hash);
    if (result != kInvalidID)
        return result;

    if (this->size() * 2 > indexSize())
        resizeHashTable(this->indexSize() * 2);

    uint32_t index = hash % indexSize();

    ID newNodeID = size();
    TableNode newNode;
    newNode.offset = m_data.size();
    newNode.size = dataSize;
    newNode.hash = hash;
    newNode.indexNext = indexPtr()[index];
    indexPtr()[index] = newNodeID;
    m_data.append(data, dataSize);
    m_data.append("", 1); // Append NUL-terminator.
    m_table.append(&newNode, sizeof(newNode));

    return newNodeID;
}

ID StringTable::id(const char *string) const
{
    size_t size = strlen(string);
    uint32_t hash;
    MurmurHash3_x86_32(string, size, 0, &hash);
    return lookup(string, size, hash);
}

ID StringTable::insert(const char *string)
{
    size_t size = strlen(string);
    uint32_t hash;
    MurmurHash3_x86_32(string, size, 0, &hash);
    return insert(string, size, hash);
}

void StringTable::resizeHashTable(uint32_t newIndexSize)
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
