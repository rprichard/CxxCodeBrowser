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

Index::Index() : m_reader(NULL), m_stringSetState(StringSetHash)
{
}

Index::Index(const std::string &path) : m_stringSetState(StringSetArray)
{
    m_reader = new Reader(path);
    uint32_t tableCount = m_reader->readUInt32();
    for (uint32_t i = 0; i < tableCount; ++i) {
        std::string tableName = m_reader->readString();
        m_stringTables[tableName] = new HashSet<char>(*m_reader);
    }
    m_stringSetBuffer = m_reader->readBuffer();
}

Index::~Index()
{
    for (auto &it : m_stringTables) {
        HashSet<char> *hashSet = it.second;
        delete hashSet;
    }

    delete m_reader;
}

void Index::save(const std::string &path)
{
    assert(m_stringSetState == StringSetArray);
    Writer writer(path);
    writer.writeUInt32(m_stringTables.size());
    for (auto &pair : m_stringTables) {
        writer.writeString(pair.first);
        pair.second->write(writer);
    }
    writer.writeBuffer(m_stringSetBuffer);
}

void Index::dump()
{
}

HashSet<char> *Index::stringTable(const std::string &name)
{
    auto it = m_stringTables.find(name);
    if (it == m_stringTables.end())
        m_stringTables[name] = new HashSet<char>();
    return m_stringTables[name];
}

void Index::addString(const char *string)
{
    assert(m_stringSetState == StringSetHash);
    m_stringSetHash.id(string);
}

// Switch the string set representation between an in-memory hash set and a
// sorted buffer.
void Index::setStringSetState(StringSetState stringSetState)
{
    if (m_stringSetState == stringSetState) {
        // Do nothing.
    } else if (stringSetState == StringSetArray) {
        assert(m_stringSetBuffer.size() == 0);
        uint32_t size = m_stringSetHash.size();
        Buffer sortedStringsBuffer(size * sizeof(uint32_t));
        uint32_t *sortedStrings = static_cast<uint32_t*>(sortedStringsBuffer.data());
        for (uint32_t i = 0; i < size; ++i)
            sortedStrings[i] = i;

        struct CompareFunc {
            HashSet<char> *m_stringSetHash;
            bool operator()(const uint32_t &x, const uint32_t &y) {
                std::pair<const char*, uint32_t> xData = m_stringSetHash->data(x);
                std::pair<const char*, uint32_t> yData = m_stringSetHash->data(y);
                int res = strncmp(xData.first, yData.first, std::min(xData.second, yData.second));
                if (res < 0)
                    return true;
                else if (res > 0)
                    return false;
                else if (xData.second < yData.second)
                    return true;
                else
                    return false;
            }
        } compareFunc;

        compareFunc.m_stringSetHash = &m_stringSetHash;

        std::sort<uint32_t*, CompareFunc>(&sortedStrings[0], &sortedStrings[size], compareFunc);

        for (uint32_t i = 0; i < size; ++i) {
            std::pair<const char*, uint32_t> data = m_stringSetHash.data(sortedStrings[i]);
            m_stringSetBuffer.append(data.first, data.second);
            m_stringSetBuffer.append("\0", 1);
        }

        m_stringSetState = StringSetArray;

    } else {
        // Not implemented.
        assert(false);
    }
}

} // namespace indexdb
