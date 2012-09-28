#include "IndexDb.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>

#include "FileIo.h"
#include "MurmurHash3.h"
#include "Util.h"

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

// Maximum number of bytes used by an encoded row of a given number of columns.
// This return value does not include the NUL terminator.
static inline size_t maxEncodedRowSize(int columnCount)
{
    return columnCount * 5;
}

static inline void encodeRow(const ID *input, int columnCount, char *output)
{
    for (int i = 0; i < columnCount; ++i) {
        uint32_t temp = input[i] + 1;
        assert(temp != 0);
        writeVleUInt32(output, temp);
    }
    *output++ = '\0';
}

static inline void decodeRow(ID *output, int columnCount, const char *input)
{
    const char *pinput = input;
    for (int i = 0; i < columnCount; ++i) {
        uint32_t temp = readVleUInt32(pinput);
        assert(temp != 0);
        output[i] = temp - 1;
    }
    assert(*pinput == '\0');
}

static inline void encodeRow(const Row &input, char *output)
{
    return encodeRow(&input[0], input.count(), output);
}

static inline void decodeRow(Row &output, const char *input)
{
    return decodeRow(&output[0], output.count(), input);
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
    encodeRow(row, m_tempEncodedRow.data());
    m_stringSetHash.insert(m_tempEncodedRow.data());
}

int Table::columnCount() const
{
    return m_columnNames.size();
}

Table::Table(Index *index, Reader &reader) : m_readonly(true)
{
    m_readonlySize = reader.readUInt32();
    uint32_t columns = reader.readUInt32();
    m_tempEncodedRow.resize(maxEncodedRowSize(columns) + 1);
    m_columnNames.resize(columns);
    for (uint32_t i = 0; i < columns; ++i) {
        m_columnNames[i] = reader.readString();
        if (!m_columnNames[i].empty()) {
            StringTable *stringTable = index->stringTable(m_columnNames[i]);
            assert(stringTable != NULL);
        }
    }
    m_stringSetBuffer = reader.readBuffer();
}

void Table::write(Writer &writer)
{
    assert(m_readonly);
    writer.writeUInt32(m_readonlySize);
    writer.writeUInt32(m_columnNames.size());
    for (auto &name : m_columnNames) {
        writer.writeString(name);
    }
    writer.writeBuffer(m_stringSetBuffer);
}

Table::Table(Index *index, const std::vector<std::string> &columnNames) :
    m_readonly(false),
    m_readonlySize(0),
    m_tempEncodedRow(maxEncodedRowSize(columnNames.size()) + 1)
{
    assert(columnNames.size() >= 1);
    uint32_t size = columnNames.size();
    m_columnNames = columnNames;
    for (uint32_t i = 0; i < size; ++i) {
        if (!m_columnNames[i].empty())
            index->addStringTable(m_columnNames[i]);
    }
}

// For efficiency, create a table mapping column numbers to the appropriate ID
// remapping table for that column, or NULL if the integers are not remapped
// (e.g. line/column numbers).
std::vector<const std::vector<ID>*> Table::createTableSpecificIdMap(
        const std::map<std::string, std::vector<ID> > &idMap)
{
    std::vector<const std::vector<ID>*> tableIdMap(columnCount());
    for (int i = 0; i < columnCount(); ++i) {
        std::string name = columnName(i);
        if (!name.empty())
            tableIdMap[i] = &idMap.at(name);
    }
    return tableIdMap;
}

// Transform the Table from its mutable representation to its read-only
// representation.
//
// The mutable representation stores rows in arbitrary order.  The read-only
// representation stores rows in sorted order.  While being sorted, the rows
// are represented unpacked.
//
// This transformation always occurs at the same time that the string tables
// are sorted.  Sorting the string tables changes their IDs, this function also
// remaps all the IDs in its rows (before sorting them, of course).  The effect
// is that everything (string and non-string tables) are in sorted order.
//
void Table::setReadOnly(
        const std::map<std::string, std::vector<ID> > &idMap)
{
    if (m_readonly)
        return;
    assert(m_stringSetBuffer.size() == 0);

    const uint32_t columnCount = this->columnCount();
    const uint32_t rowCount = m_stringSetHash.size();
    std::vector<ID> tableData(rowCount * columnCount);

    {
        // Decode the packed strings in the string set into a 2-dimensional
        // array.  As we're writing the new table, transform the ID values
        // using the idMap.
        auto tableIdMap = createTableSpecificIdMap(idMap);
        uint32_t rowIndex = 0;
        ID *ptr = tableData.data();
        ID *endPtr = tableData.data() + tableData.size();
        while (ptr != endPtr) {
            decodeRow(ptr, columnCount, m_stringSetHash.item(rowIndex));
            for (uint32_t column = 0; column < columnCount; ++column) {
                const std::vector<ID> *map = tableIdMap[column];
                if (map != NULL)
                    *ptr = (*map)[*ptr];
                // Convert each uint32_t to big-endian so that rows can be
                // compared below using memcmp.
                *ptr = HostToBE32(*ptr);
                ptr++;
            }
            rowIndex++;
        }
        // Discard the old string set to conserve memory.
        m_stringSetHash = StringTable();
    }

    // Create a table mapping sorted index to old index.
    std::vector<uint32_t> sortedStrings(rowCount);
    for (uint32_t i = 0; i < rowCount; ++i)
        sortedStrings[i] = i;

    struct CompareFunc {
        uint32_t rowCount;
        uint32_t columnCount;
        ID *tableData;
        bool operator()(uint32_t x, uint32_t y) {
            assert(x < rowCount);
            assert(y < rowCount);
            ID *rowX = &tableData[x * columnCount];
            ID *rowY = &tableData[y * columnCount];
            return memcmp(rowX, rowY, columnCount * sizeof(ID)) < 0;
        }
    } compareFunc;

    compareFunc.rowCount = rowCount;
    compareFunc.columnCount = columnCount;
    compareFunc.tableData = tableData.data();
    std::sort<uint32_t*, CompareFunc>(
                &sortedStrings[0],
                &sortedStrings[rowCount],
                compareFunc);

    // Prepend an initial NUL character to simplify iterator decrement.
    m_stringSetBuffer.append("", 1);

    // Encode each row and add it to the buffer.
    std::vector<char> encodedRow(maxEncodedRowSize(columnCount) + 1);
    for (uint32_t newIndex = 0; newIndex < rowCount; ++newIndex) {
        uint32_t oldIndex = sortedStrings[newIndex];
        ID *row = &tableData[oldIndex * columnCount];
        for (uint32_t i = 0; i < columnCount; ++i) {
            // The integers were converted to big-endian above.  Convert them
            // to host-encoding here.  (encodeRow will further convert them to
            // a VLE representation.)
            row[i] = BEToHost32(row[i]);
        }
        encodeRow(row, columnCount, encodedRow.data());
        m_stringSetBuffer.append(
                    encodedRow.data(),
                    strlen(encodedRow.data()) + 1);
    }

    m_readonly = true;
    m_readonlySize = rowCount;
}

// Find the first iterator that is greater than or equal to the given row.
TableIterator Table::lowerBound(const Row &row)
{
    assert(m_readonly);
    assert(static_cast<size_t>(row.count()) <= columnCount());

    TableIterator itMin = begin();
    TableIterator itMax = end();
    Row tempRow(columnCount());

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

        decodeRow(tempRow, itMid.m_string);
        if (tempRow < row) {
            itMin = itMid;
            ++itMin;
        } else {
            itMax = itMid;
        }
    }

    return itMin;
}

void Table::dumpStats() const
{
#if STRING_TABLE_STATS
    m_stringSetHash.dumpStats();
#endif
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
    std::map<std::string, std::vector<ID> > idMap;

    // For each string table in "other", add all of the strings to the
    // corresponding string table in "this", while also building a table
    // mapping each of the "other" IDs into "this" IDs.
    for (auto pair : other.m_stringTables) {
        idMap[pair.first] = std::vector<ID>();
        std::vector<ID> &stringTableIdMap = idMap[pair.first];
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
        std::map<std::string, std::vector<ID> > &idMap)
{
    int columnCount = srcTable->columnCount();
    auto tableIdMap = srcTable->createTableSpecificIdMap(idMap);

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

std::pair<StringTable, std::vector<ID> >
Index::sortStringTable(const StringTable &input)
{
    const uint32_t stringCount = input.size();

    // Construct a table of sorted indexes to original indexes.
    std::vector<uint32_t> sortedStrings(stringCount);
    for (uint32_t i = 0; i < stringCount; ++i)
        sortedStrings[i] = i;
    struct CompareFunc {
        const StringTable *input;
        bool operator()(uint32_t x, uint32_t y) {
            return strcmp(input->item(x), input->item(y)) < 0;
        }
    };
    CompareFunc func;
    func.input = &input;
    std::sort(&sortedStrings[0], &sortedStrings[stringCount], func);

    // Copy each string into the new StringTable.
    StringTable newTable;
    std::vector<ID> idMap(stringCount);
    for (uint32_t newIndex = 0; newIndex < stringCount; ++newIndex) {
        uint32_t oldIndex = sortedStrings[newIndex];
        idMap[oldIndex] = newIndex;
        newTable.insert(
                    input.item(oldIndex),
                    input.itemSize(oldIndex),
                    input.itemHash(oldIndex));
    }

    return std::make_pair(std::move(newTable), std::move(idMap));
}

size_t Index::stringTableCount()
{
    return m_stringTables.size();
}

std::string Index::stringTableName(size_t index)
{
    auto it = m_stringTables.begin(), itEnd = m_stringTables.end();
    while (assert(it != itEnd), index > 0) {
        ++it;
        --index;
    }
    return it->first;
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

size_t Index::tableCount()
{
    return m_tables.size();
}

std::string Index::tableName(size_t index)
{
    auto it = m_tables.begin(), itEnd = m_tables.end();
    while (assert(it != itEnd), index > 0) {
        ++it;
        --index;
    }
    return it->first;
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
    if (m_readonly)
        return;

    m_readonly = true;

    // Sort the string tables.
    std::map<std::string, std::vector<ID> > idMap;
    for (std::pair<std::string, StringTable*> table : m_stringTables) {
        std::pair<StringTable, std::vector<ID> > pair =
                sortStringTable(*table.second);
        *table.second = std::move(pair.first);
        idMap[table.first] = std::move(pair.second);
    }

    // Transform the tables themselves and update them with the new string IDs.
    for (auto table : m_tables) {
        table.second->setReadOnly(idMap);
    }
}

} // namespace indexdb
