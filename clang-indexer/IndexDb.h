#ifndef INDEXDB_H
#define INDEXDB_H

#include <stdint.h>
#include <string>
#include <utility>
#include "HashSet.h"
#include <unordered_set>
#include <unordered_map>

namespace indexdb {

class Reader;

struct Pair {
    ID key;
    ID value;
};

inline bool operator<(const Pair &x, const Pair &y) {
    if (x.key < y.key)
        return true;
    else if (x.key > y.key)
        return false;
    else if (x.value < y.value)
        return true;
    else
        return false;
}

inline bool operator==(const Pair &x, const Pair &y) {
    return x.key == y.key && x.value == y.value;
}

struct PairHashFunc {
    size_t operator()(const Pair &pair) const {
        return pair.key * 41 + pair.value;
    }
};

class Index {
public:
    enum PairState { PairHash, PairArray };

    // Operations on the index as a whole.
    Index();
    explicit Index(const std::string &path);
    ~Index();
    void merge(const Index &other);
    void save(const std::string &path);
    void dump();

    // Add/query values.
    bool isValidID(ID id) const;
    ID stringID(const char *data);
    ID stringID(const char *data, uint32_t size);
    std::pair<const char*, uint32_t> stringData(ID id) const;
    ID tupleID(const ID *data, uint32_t size);
    std::pair<const ID*, uint32_t> tupleData(ID id) const;
    void addPair(ID key, ID value);
    std::pair<const Pair*, uint32_t> lookupPair(ID key) const;
    void setPairState(PairState pairState);

private:
    uint32_t hashTupleData(const ID *data, uint32_t count);
    uint32_t pairCount() const {
        return m_pairBuffer.size() / sizeof(Pair);
    }
    Pair &pair(uint32_t index) {
        return static_cast<Pair*>(m_pairBuffer.data())[index];
    }
    const Pair &pair(uint32_t index) const {
        return static_cast<const Pair*>(m_pairBuffer.data())[index];
    }

    // TODO: Is it faster if we can avoid this pointer indirection?
    HashSet<char> *m_stringSet;
    HashSet<ID> *m_tupleSet;
    std::unordered_set<Pair, PairHashFunc> m_pairHash;
    Buffer m_pairBuffer;
    PairState m_pairState;
    Reader *m_reader;
};

} // namespace indexdb

#endif // INDEXDB_H
