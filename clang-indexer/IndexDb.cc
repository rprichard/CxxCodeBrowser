#include "IndexDb.h"
#include "MurmurHash3.h"
#include "FileIo.h"
#include <functional>
#include <iostream>
#include <vector>
#include <algorithm>

// UNIX headers
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace indexdb {

const ID kTupleIDOffset = 0x80000000u;

struct IndexHeader {
    uint32_t stringSetOffset;
    uint32_t tupleSetOffset;
    uint32_t pairBufferOffset;
    uint32_t pairBufferSize;
};

Index::Index() : m_pairState(PairHash), m_reader(NULL)
{
    m_stringSet = new HashSet<char>();
    m_tupleSet = new HashSet<ID>();
}

Index::Index(const std::string &path) : m_pairState(PairArray)
{
    m_reader = new Reader(path);
    m_stringSet = new HashSet<char>(*m_reader);
    m_tupleSet = new HashSet<ID>(*m_reader);
    m_pairBuffer = m_reader->readBuffer();
}

Index::~Index()
{
    delete m_stringSet;
    delete m_tupleSet;
    delete m_reader;
}

void Index::merge(const Index &other)
{
    ID *remappedStringIDs = new ID[other.m_stringSet->size()];
    ID *remappedTupleIDs = new ID[other.m_tupleSet->size()];

    // Merge strings.
    for (ID otherStringID = 0;
            otherStringID < other.m_stringSet->size();
            ++otherStringID) {
        std::pair<const char*, uint32_t> data =
            other.m_stringSet->data(otherStringID);
        ID thisStringID = m_stringSet->id(
            data.first,
            data.second,
            other.m_stringSet->hash(otherStringID));
        remappedStringIDs[otherStringID] = thisStringID;
    }

    // Merge tuples.
    std::vector<ID> tempTuple;
    for (ID otherTupleID = 0;
            otherTupleID < other.m_tupleSet->size();
            ++otherTupleID) {
        std::pair<const ID*, uint32_t> data =
            other.m_tupleSet->data(otherTupleID);
        uint32_t tupleSize = data.second;
        tempTuple.resize(tupleSize);
        for (uint32_t i = 0; i < tupleSize; ++i) {
            tempTuple[i] = remappedStringIDs[data.first[i]];
        }
        ID thisTupleID = tupleID(tempTuple.data(), tupleSize);
        remappedTupleIDs[otherTupleID] = thisTupleID;
    }

    // Merge the pair data.
    setPairState(PairHash);
    assert(other.m_pairState == PairArray);
    for (uint32_t i = 0; i < other.pairCount(); ++i) {
        // TODO: Figure out how to deal with string-vs-tuple IDs reliably
        // (e.g. type safety).
        const Pair &p = other.pair(i);
        assert(other.isValidID(p.key));
        assert(other.isValidID(p.value));
        ID thisKey = (p.key < kTupleIDOffset) ?
                    remappedStringIDs[p.key] :
                    (remappedTupleIDs[p.key - kTupleIDOffset]);
        ID thisValue = (p.value < kTupleIDOffset) ?
                    remappedStringIDs[p.value] :
                    (remappedTupleIDs[p.value - kTupleIDOffset]);
        addPair(thisKey, thisValue);
    }

    delete [] remappedStringIDs;
    delete [] remappedTupleIDs;
}

void Index::save(const std::string &path)
{
    assert(m_pairState == PairArray);
    Writer writer(path);
    m_stringSet->write(writer);
    m_tupleSet->write(writer);
    writer.writeBuffer(m_pairBuffer);
}

void Index::dump()
{
    std::cout << m_stringSet->size() << std::endl;
    std::cout << m_tupleSet->size() << std::endl;
    std::cout << pairCount() << std::endl;
}

bool Index::isValidID(ID id) const
{
    if (id < kTupleIDOffset)
        return id < m_stringSet->size();
    else
        return (id - kTupleIDOffset) < m_tupleSet->size();
}

ID Index::stringID(const char *data)
{
    return stringID(data, strlen(data));
}

ID Index::stringID(const char *data, uint32_t size)
{
    uint32_t hash;
    MurmurHash3_x86_32(data, size, 0, &hash);
    return m_stringSet->id(data, size, hash);
}

std::pair<const char*, uint32_t> Index::stringData(ID id) const
{
    assert(id < m_stringSet->size());
    return m_stringSet->data(id);
}

uint32_t Index::hashTupleData(const ID *data, uint32_t size)
{
    uint32_t result = 0;
    for (uint32_t i = 0; i < size; ++i) {
        // We could use either the ID or the hash of the string identified by
        // the ID.  I'll use the latter, which makes the tuple hash ID stable
        // across merging.  I'm not sure whether that matters.
        assert(data[i] < m_stringSet->size());
        result = result * 41 + m_stringSet->hash(data[i]);
    }
    return result;
}

ID Index::tupleID(const ID *data, uint32_t size)
{
    uint32_t hash = hashTupleData(data, size);
    return m_tupleSet->id(data, size, hash) + kTupleIDOffset;
}

std::pair<const ID*, uint32_t> Index::tupleData(ID id) const
{
    assert(id >= kTupleIDOffset);
    assert(id - kTupleIDOffset < m_tupleSet->size());
    return m_tupleSet->data(id - kTupleIDOffset);
}

// Add a pair to the pair hash.
void Index::addPair(ID key, ID value)
{
    assert(m_pairState == PairHash);
    assert(isValidID(key));
    assert(isValidID(value));
    Pair pair = { key, value };
    m_pairHash.insert(pair);
}

// Lookup pair values corresponding to the key.
std::pair<const Pair*, uint32_t> Index::lookupPair(ID key) const
{
    // The return type of this function implies that all of the pairs must be
    // in the sorted pair buffer.
    assert(m_pairState == PairArray);

    // Find the lowest entry matching the key using binary search.
    uint32_t iMin = 0;
    uint32_t iMax = pairCount() - 1;
    while (iMin < iMax) {
        uint32_t iMid = iMin + (iMax - iMax) / 2;
        assert(iMid < iMax);
        if (pair(iMid).key < key) {
            iMin = iMid + 1;
        } else if (pair(iMid).key > key) {
            iMax = iMid - 1;
        } else {
            // iMid will always be less then iMax, which is important because
            // otherwise the search could get stuck in an infinite loop here.
            iMax = iMid;
        }
    }
    if (iMin != iMax)
        return std::pair<const Pair*, uint32_t>(NULL, 0);

    // Find the last entry matching this key.
    while (iMax + 1 < pairCount() && pair(iMax + 1).key == key)
        iMax++;

    return std::pair<const Pair*, uint32_t>(&pair(iMin), iMax - iMin + 1);
}

// Switch the pair representation between an in-memory hash table and a sorted
// buffer.
void Index::setPairState(PairState pairState)
{
    if (m_pairState == pairState) {
        // Do nothing.
    } else if (pairState == PairArray) {
        assert(m_pairBuffer.size() == 0);
        m_pairBuffer = Buffer(sizeof(Pair) * m_pairHash.size());
        uint32_t pairIndex = 0;
        for (auto &hashedPair : m_pairHash) {
            pair(pairIndex++) = hashedPair;
        }
        std::sort(&pair(0), &pair(pairCount()));
        m_pairHash.clear();
        m_pairState = PairArray;
    } else {
        // Not implemented.
        assert(false);
    }
}

} // namespace indexdb
