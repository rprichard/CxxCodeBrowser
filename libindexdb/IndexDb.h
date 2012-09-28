#ifndef INDEXDB_H
#define INDEXDB_H

#include <stdint.h>

#include <cassert>
#include <cstring>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "StringTable.h"

namespace indexdb {

class Writer;
class Reader;
class Index;
class Table;


///////////////////////////////////////////////////////////////////////////////
// Row

class Row {
public:
    explicit Row(int count) {
        assert(count > 0);
        m_data = static_cast<uint32_t*>(malloc(count * sizeof(uint32_t)));
        assert(m_data != NULL);
        m_count = count;
    }

    ~Row() {
        free(m_data);
    }

    int count() const { return m_count; }
    uint32_t &operator[](size_t i) { return m_data[i]; }
    const uint32_t &operator[](size_t i) const { return m_data[i]; }
    void resize(int columns);

private:
    uint32_t *m_data;
    int m_count;
};

// A shorter row comes before a larger row if the common columns' values are
// equal.
inline bool operator<(const Row &x, const Row &y) {
    int minColumn = std::min(x.count(), y.count());
    for (int column = 0; column < minColumn; ++column) {
        if (x[column] < y[column])
            return true;
        else if (x[column] > y[column])
            return false;
    }
    if (x.count() < y.count())
        return true;
    return false;
}


///////////////////////////////////////////////////////////////////////////////
// TableIterator

class TableIterator {
public:
    explicit TableIterator(const Table *table, const char *string) : m_table(table), m_string(string) {}
    TableIterator &operator++() { m_string += strlen(m_string) + 1; return *this; }
    TableIterator &operator--();
    bool operator!=(const TableIterator &other) { return m_string != other.m_string; }
    bool operator==(const TableIterator &other) { return m_string == other.m_string; }
    bool operator<(const TableIterator &other) { return m_string < other.m_string; }
    bool operator<=(const TableIterator &other) { return m_string <= other.m_string; }
    bool operator>(const TableIterator &other) { return m_string > other.m_string; }
    bool operator>=(const TableIterator &other) { return m_string >= other.m_string; }
    void value(Row &row);

private:
    const Table *m_table;
    const char *m_string;

    friend class Table;
};


///////////////////////////////////////////////////////////////////////////////
// Table

class Table {
public:
    typedef TableIterator iterator;

    void add(const Row &row);
    int columnCount() const;

    TableIterator begin() const {
        assert(m_readonly);
        return TableIterator(this, static_cast<const char*>(
                                 m_stringSetBuffer.data()) + 1);
    }

    TableIterator end() const {
        assert(m_readonly);
        return TableIterator(this, static_cast<const char*>(
                                 m_stringSetBuffer.data()) +
                                 m_stringSetBuffer.size());
    }

    std::string columnName(int i) const {
        return m_columnNames[i];
    }

    TableIterator lowerBound(const Row &row);
    void dumpStats() const;

    uint32_t bufferSize() const {
        assert(m_readonly);
        return m_stringSetBuffer.size();
    }

private:
    Table(Index *index, Reader &reader);
    void write(Writer &writer);
    Table(Index *index, const std::vector<std::string> &columns);
    std::vector<const std::vector<ID>*> createTableSpecificIdMap(
            const std::map<std::string, std::vector<ID> > &idMap);
    void setReadOnly(const std::map<std::string, std::vector<ID> > &idMap);

    bool m_readonly;
    std::vector<std::string> m_columnNames;
    Buffer m_stringSetBuffer;
    StringTable m_stringSetHash;
    std::vector<char> m_tempEncodedRow;

    friend class Index;
};


///////////////////////////////////////////////////////////////////////////////
// Index

class Index {
public:

    // Operations on the index as a whole.
    Index();
    explicit Index(const std::string &path);
    ~Index();
    void save(const std::string &path);
    void merge(const Index &other);

    // Add/query values.
    size_t stringTableCount();
    std::string stringTableName(size_t index);
    StringTable *addStringTable(const std::string &name);
    StringTable *stringTable(const std::string &name);
    const StringTable *stringTable(const std::string &name) const;
    size_t tableCount();
    std::string tableName(size_t index);
    Table *addTable(const std::string &name, const std::vector<std::string> &names);
    Table *table(const std::string &name);
    const Table *table(const std::string &name) const;
    void setReadOnly();

private:
    void mergeTable(
            Table *destTable,
            Table *srcTable,
            std::map<std::string, std::vector<indexdb::ID> > &idMap);
    std::pair<StringTable, std::vector<ID> > sortStringTable(
            const StringTable &input);

    Reader *m_reader;

    bool m_readonly;
    std::map<std::string, StringTable*> m_stringTables;
    std::map<std::string, Table*> m_tables;
};

} // namespace indexdb

#endif // INDEXDB_H
