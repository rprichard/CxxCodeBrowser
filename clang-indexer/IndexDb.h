#ifndef INDEXDB_H
#define INDEXDB_H

#include <stdint.h>
#include <string>
#include <utility>
#include "HashSet.h"
#include <map>
#include <unordered_set>
#include <unordered_map>

namespace indexdb {

class Reader;

class Index {
public:
    class iterator {
    public:
        explicit iterator(const char *string) : string(string) {}
        const char *operator*() { return string; }
        iterator &operator++() { string += strlen(string) + 1; return *this; }
        bool operator!=(const iterator other) { return string != other.string; }
        bool operator==(const iterator other) { return string == other.string; }
        bool operator<(const iterator other) { return string < other.string; }
        bool operator<=(const iterator other) { return string <= other.string; }
        bool operator>(const iterator other) { return string > other.string; }
        bool operator>=(const iterator other) { return string >= other.string; }

    private:
        const char *string;
    };

    enum StringSetState { StringSetHash, StringSetArray };

    // Operations on the index as a whole.
    Index();
    explicit Index(const std::string &path);
    ~Index();
    void save(const std::string &path);
    void dump();

    // Add/query values.
    HashSet<char> *stringTable(const std::string &name);
    void addString(const char *string);
    void setStringSetState(StringSetState pairState);
    iterator begin() const {
        assert(m_stringSetState == StringSetArray);
        return iterator(static_cast<const char*>(m_stringSetBuffer.data()));
    }
    iterator end() {
        assert(m_stringSetState == StringSetArray);
        return iterator(static_cast<const char*>(m_stringSetBuffer.data()) +
                        m_stringSetBuffer.size());
    }

private:
    Reader *m_reader;

    StringSetState m_stringSetState;
    std::map<std::string, HashSet<char>*> m_stringTables;
    HashSet<char> m_stringSetHash;
    Buffer m_stringSetBuffer;
};

} // namespace indexdb

#endif // INDEXDB_H
