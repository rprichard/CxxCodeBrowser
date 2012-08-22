#include "IndexDb.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>

#include "FileIo.h"
#include "MurmurHash3.h"

namespace indexdb {


///////////////////////////////////////////////////////////////////////////////
// Row

static inline uint32_t readVleUInt32(const char *&buffer)
{
    uint32_t result = 0;
    unsigned char nibble;
    int bit = 0;
    do {
        nibble = static_cast<unsigned char>(*(buffer++));
        result |= (nibble & 0x7F) << bit;
        bit += 7;
    } while ((nibble & 0x80) == 0x80);
    return result;
}

// This encoding of a uint32 is guaranteed not to contain NUL bytes unless the
// value is zero.
static inline void writeVleUInt32(char *&buffer, uint32_t value)
{
    do {
        unsigned char nibble = value & 0x7F;
        value >>= 7;
        if (value != 0)
            nibble |= 0x80;
        *buffer++ = nibble;
    } while (value != 0);
}

static inline void encodeRow(const Row &input, char *output)
{
    for (int i = 0; i < input.count(); ++i) {
        uint32_t temp = input[i] + 1;
        assert(temp != 0);
        writeVleUInt32(output, temp);
    }
    *output++ = '\0';
}

static inline void decodeRow(Row &output, const char *input)
{
    const char *pinput = input;
    for (int i = 0; i < output.count(); ++i) {
        uint32_t temp = readVleUInt32(pinput);
        assert(temp != 0);
        output[i] = temp - 1;
    }
    assert(*pinput == '\0');
}

void Row::resize(int count)
{
    if (m_count != count) {
        assert(count > 0);
        m_data = static_cast<uint32_t*>(
                    realloc(m_data, count * sizeof(uint32_t)));
        assert(m_data != NULL);
        m_count = count;
    }
}


///////////////////////////////////////////////////////////////////////////////
// TableIterator

void TableIterator::value(Row &row)
{
    row.resize(m_table->columnCount());
    decodeRow(row, m_string);
}

TableIterator &TableIterator::operator--()
{
    const char *start = m_table->begin().m_string;
    assert(m_string - start >= 2);
    assert(m_string[-1] == '\0');
    m_string -= 2;
    while (true) {
        if (m_string[0] == '\0') {
            m_string++;
            break;
        }
        m_string--;
    }
    return *this;
}


///////////////////////////////////////////////////////////////////////////////
// Table


void Table::add(const Row &row)
{
    assert(!m_readonly);
    char buffer[256]; // TODO: handle arbitrary row sizes
    encodeRow(row, buffer);
    m_stringSetHash.insert(buffer);
}

int Table::columnCount() const
{
    return m_columnNames.size();
}

Table::Table(Index *index, Reader &reader) : m_readonly(true)
{
    uint32_t columns = reader.readUInt32();
    m_columnNames.resize(columns);
    m_columnStringSets.resize(columns);
    for (uint32_t i = 0; i < columns; ++i) {
        m_columnNames[i] = reader.readString();
        if (m_columnNames[i].empty()) {
            m_columnStringSets[i] = NULL;
        } else {
            m_columnStringSets[i] = index->stringTable(m_columnNames[i]);
            assert(m_columnStringSets[i] != NULL);
        }
    }
    m_stringSetBuffer = reader.readBuffer();
}

void Table::write(Writer &writer)
{
    assert(m_readonly);
    writer.writeUInt32(m_columnNames.size());
    for (auto &name : m_columnNames) {
        writer.writeString(name);
    }
    writer.writeBuffer(m_stringSetBuffer);
}

Table::Table(Index *index, const std::vector<std::string> &columnNames) :
    m_readonly(false)
{
    assert(columnNames.size() >= 1);
    uint32_t size = columnNames.size();
    m_columnNames = columnNames;
    m_columnStringSets.resize(size);
    for (uint32_t i = 0; i < size; ++i) {
        m_columnStringSets[i] = index->addStringTable(m_columnNames[i]);
    }
}

void Table::setReadOnly()
{
    if (m_readonly)
        return;

    assert(m_stringSetBuffer.size() == 0);
    uint32_t size = m_stringSetHash.size();
    Buffer sortedStringsBuffer(size * sizeof(uint32_t));
    uint32_t *sortedStrings = static_cast<uint32_t*>(sortedStringsBuffer.data());
    for (uint32_t i = 0; i < size; ++i)
        sortedStrings[i] = i;

    struct CompareFunc {
        StringTable *m_stringSetHash;
        bool operator()(const uint32_t &x, const uint32_t &y) {
            return strcmp(m_stringSetHash->item(x),
                          m_stringSetHash->item(y)) < 0;
        }
    } compareFunc;

    compareFunc.m_stringSetHash = &m_stringSetHash;

    std::sort<uint32_t*, CompareFunc>(&sortedStrings[0], &sortedStrings[size], compareFunc);

    // Prepend an initial NUL character to simplify iterator decrement.
    m_stringSetBuffer.append("", 1);

    for (uint32_t i = 0; i < size; ++i) {
        // The itemSize does not include the NUL-terminator, but item's result
        // is guaranteed to be NUL-terminated.  Add one to the size so the
        // string in m_stringSetBuffer is also NUL-terminated.
        m_stringSetBuffer.append(
                    m_stringSetHash.item(sortedStrings[i]),
                    m_stringSetHash.itemSize(sortedStrings[i]) + 1);
    }

    m_readonly = true;
}

// Find the first iterator that is greater than or equal to the given row.
TableIterator Table::lowerBound(const Row &row)
{
    char buffer[256]; // TODO: allow arbitary size
    assert(static_cast<size_t>(row.count()) <= m_columnNames.size());
    encodeRow(row, buffer);

    TableIterator itMin = begin();
    TableIterator itMax = end();

    // Binary search for the provided row.
    while (itMin != itMax) {
        // Find a midpoint itMid.  It will be in the range [itMin, itMax).
        TableIterator itMid = itMin;
        {
            const char *midStr = itMin.m_string + ((itMax.m_string - itMin.m_string) / 2);
            assert(midStr < itMax.m_string);
            midStr += strlen(midStr) + 1;
            itMid.m_string = midStr;
            --itMid;
            assert(itMid >= itMin && itMid < itMax);
        }

        int cmp = strcmp(itMid.m_string, buffer);
        if (cmp < 0) {
            itMin = itMid;
            ++itMin;
        } else {
            itMax = itMid;
        }
    }

    return itMin;
}


///////////////////////////////////////////////////////////////////////////////
// Index

Index::Index() : m_reader(NULL), m_readonly(false)
{
}

Index::Index(const std::string &path) : m_readonly(true)
{
    m_reader = new Reader(path);

    uint32_t tableCount;

    tableCount = m_reader->readUInt32();
    for (uint32_t i = 0; i < tableCount; ++i) {
        std::string tableName = m_reader->readString();
        m_stringTables[tableName] = new StringTable(*m_reader);
    }

    tableCount = m_reader->readUInt32();
    for (uint32_t i = 0; i < tableCount; ++i) {
        std::string tableName = m_reader->readString();
        m_tables[tableName] = new Table(this, *m_reader);
    }
}

Index::~Index()
{
    for (auto &it : m_stringTables) {
        StringTable *hashSet = it.second;
        delete hashSet;
    }

    for (auto &it : m_tables) {
        Table *table = it.second;
        delete table;
    }

    delete m_reader;
}

void Index::save(const std::string &path)
{
    assert(m_readonly);
    Writer writer(path);
    writer.writeUInt32(m_stringTables.size());
    for (auto &pair : m_stringTables) {
        writer.writeString(pair.first);
        pair.second->write(writer);
    }
    writer.writeUInt32(m_tables.size());
    for (auto &pair : m_tables) {
        writer.writeString(pair.first);
        pair.second->write(writer);
    }
}

// Merge all of the string tables and tables from the other index into the
// current index.  String tables and tables are created if they do not exist.
// A table must have the same number and name of columns in the two indices.
void Index::merge(const Index &other)
{
    std::map<std::string, std::vector<indexdb::ID> > idMap;

    // For each string table in "other", add all of the strings to the
    // corresponding string table in "this", while also building a table
    // mapping each of the "other" IDs into "this" IDs.
    for (auto pair : other.m_stringTables) {
        idMap[pair.first] = std::vector<indexdb::ID>();
        std::vector<indexdb::ID> &stringTableIdMap = idMap[pair.first];
        StringTable *destStringTable = addStringTable(pair.first);
        StringTable *srcStringTable = pair.second;
        stringTableIdMap.resize(srcStringTable->size());
        for (size_t srcID = 0; srcID < srcStringTable->size(); ++srcID) {
            ID destID = destStringTable->insert(
                        srcStringTable->item(srcID),
                        srcStringTable->itemSize(srcID),
                        srcStringTable->itemHash(srcID));
            stringTableIdMap[srcID] = destID;
        }
    }

    // For each row in each "other" table, add the row to the corresponding
    // table in "this", after remapping IDs.
    for (auto tablePair : other.m_tables) {
        Table *srcTable = tablePair.second;
        Table *destTable = addTable(tablePair.first, srcTable->m_columnNames);
        mergeTable(destTable, srcTable, idMap);
    }
}

void Index::mergeTable(
        Table *destTable,
        Table *srcTable,
        std::map<std::string, std::vector<indexdb::ID> > &idMap)
{
    int columnCount = srcTable->columnCount();

    // For efficiency, create a table mapping column numbers to the appropriate
    // ID remapping table for that column, or NULL if the integers are not
    // remapped (e.g. line/column numbers).
    std::vector<std::vector<indexdb::ID>*> tableIdMap;
    tableIdMap.resize(columnCount);
    for (int i = 0; i < columnCount; ++i) {
        std::string name = srcTable->columnName(i);
        if (!name.empty())
            tableIdMap[i] = &idMap[name];
    }

    // Add each of the source table's rows to the destination table.
    Row row(columnCount);
    for (TableIterator it = srcTable->begin(), itEnd = srcTable->end();
            it != itEnd;
            ++it) {
        it.value(row);
        for (int i = 0; i < columnCount; ++i) {
            if (tableIdMap[i] != NULL) {
                uint32_t temp = row[i];
                assert(temp < tableIdMap[i]->size());
                row[i] = (*tableIdMap[i])[temp];
            }
        }
        destTable->add(row);
    }
}

// Returns the string table with the given name.  Creates the table if it does
// not exist.  The index must be writable to call this method.
StringTable *Index::addStringTable(const std::string &name)
{
    assert(!m_readonly);
    auto it = m_stringTables.find(name);
    if (it != m_stringTables.end())
        return it->second;
    m_stringTables[name] = new StringTable();
    return m_stringTables[name];
}

// Returns the string table with the given name or NULL if it does not exist.
StringTable *Index::stringTable(const std::string &name)
{
    auto it = m_stringTables.find(name);
    return (it != m_stringTables.end()) ? it->second : NULL;
}

// Returns the string table with the given name or NULL if it does not exist.
const StringTable *Index::stringTable(const std::string &name) const
{
    auto it = m_stringTables.find(name);
    return (it != m_stringTables.end()) ? it->second : NULL;
}

// Returns the table with the given name, creating it if it does not exist.  If
// it already exists, then its column names must match the given ones.  The
// index must be writable to call this method.
Table *Index::addTable(const std::string &name, const std::vector<std::string> &names)
{
    assert(!m_readonly);
    auto it = m_tables.find(name);
    if (it != m_tables.end()) {
        assert(it->second->m_columnNames == names);
        return it->second;
    }
    assert(m_tables.find(name) == m_tables.end());
    m_tables[name] = new Table(this, names);
    return m_tables[name];
}

// Return the table with the given name or NULL if it does not exist.
const Table *Index::table(const std::string &name) const
{
    auto it = m_tables.find(name);
    return (it != m_tables.end()) ? it->second : NULL;
}

// Return the table with the given name or NULL if it does not exist.
Table *Index::table(const std::string &name)
{
    auto it = m_tables.find(name);
    return (it != m_tables.end()) ? it->second : NULL;
}

void Index::setReadOnly()
{
    m_readonly = true;
    for (auto table : m_tables) {
        table.second->setReadOnly();
    }
}

} // namespace indexdb
