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
    // This class's in-memory representation has the same endianness as the
    // string table's on-disk representation, but its methods operate on
    // integers with normal host endianness.  The intent is that a table can be
    // memory-mapped and cast to an EncodedUInt32*, and correct endianness
    // handling will be ensured.
    //
    // It also means that clients can sometimes avoid superfluous byte-swapping
    // by using the EncodedUInt32 type instead of the uint32_t type, without
    // danger of getting the number of swaps wrong.
    class EncodedUInt32 {
    public:
        EncodedUInt32() {}
        EncodedUInt32(uint32_t val) : m_val(HostToLE32(val)) {}
        uint32_t get() const { return LEToHost32(m_val); }
        operator uint32_t() const { return get(); }
    private:
        uint32_t m_val;
    };

    typedef EncodedUInt32 EncodedID;

    struct TableNode {
        EncodedUInt32 offset;
        EncodedUInt32 size;
        EncodedUInt32 hash;
        EncodedID indexNext;
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

    EncodedID *indexPtr()             { return static_cast<EncodedID*>(m_index.data()); }
    const EncodedID *indexPtr() const { return static_cast<const EncodedID*>(m_index.data()); }

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
    uint32_t contentByteSize() const { return m_data.size(); }
    Buffer pillageContent() { return std::move(m_data); }

    friend class Index;
};

} // namespace indexdb

#endif // INDEXDB_STRINGTABLE_H
