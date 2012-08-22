#ifndef INDEXDB_H
#define INDEXDB_H

#include <stdint.h>
#include <cassert>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "HashSet.h"

namespace indexdb {

class Writer;
class Reader;
class Index;
class Table;


///////////////////////////////////////////////////////////////////////////////
// Row

class Row {
public:
    Row(int count) {
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


///////////////////////////////////////////////////////////////////////////////
// TableIterator

class TableIterator {
public:
    explicit TableIterator(const Table *table, const char *string) : m_table(table), m_string(string) {}
    TableIterator &operator++() { m_string += strlen(m_string) + 1; return *this; }
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
        return TableIterator(this, static_cast<const char*>(m_stringSetBuffer.data()));
    }

    TableIterator end() const {
        assert(m_readonly);
        return TableIterator(this, static_cast<const char*>(m_stringSetBuffer.data()) +
                                                            m_stringSetBuffer.size());
    }

    std::string columnName(int i) const {
        return m_columnNames[i];
    }

private:
    Table(Index *index, Reader &reader);
    void write(Writer &writer);
    Table(Index *index, const std::vector<std::string> &columns);
    void setReadOnly();

    bool m_readonly;
    std::vector<std::string> m_columnNames;
    std::vector<HashSet<char>*> m_columnStringSets;
    Buffer m_stringSetBuffer;
    HashSet<char> m_stringSetHash;

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
    void merge(Index &other);
    void dump();

    // Add/query values.
    HashSet<char> *addStringTable(const std::string &name);
    HashSet<char> *stringTable(const std::string &name);
    const HashSet<char> *stringTable(const std::string &name) const;
    Table *addTable(const std::string &name, const std::vector<std::string> &names);
    Table *table(const std::string &name);
    const Table *table(const std::string &name) const;
    void setReadOnly();

private:
    void mergeTable(
            Table *destTable,
            Table *srcTable,
            std::map<std::string, std::vector<indexdb::ID> > &idMap);

    Reader *m_reader;

    bool m_readonly;
    std::map<std::string, HashSet<char>*> m_stringTables;
    std::map<std::string, Table*> m_tables;
};

} // namespace indexdb

#endif // INDEXDB_H
