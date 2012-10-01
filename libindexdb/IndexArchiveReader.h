#ifndef INDEXDB_INDEXARCHIVEREADER_H
#define INDEXDB_INDEXARCHIVEREADER_H

#include <stdint.h>
#include <map>
#include <string>
#include <vector>

namespace indexdb {

class Index;

// An index archive is a file containing a series of indexes.
//
// Its intended application is to speed up merging of indices for C/C++
// translation units.  Each source file in a translation unit is placed into a
// separate index within a single index archive file.  Each index is hashed
// using something like MD5/SHA.  When merging index archives, an entry can be
// skipped if a previous entry with the same hash was already merged.  The
// expectation is that entries for header files will typically be identical
// between translation units.
class IndexArchiveReader
{
public:
    struct Entry {
        std::string name;
        std::string hash;
        uint64_t fileOffset;
        uint64_t fileLength;
    };

    IndexArchiveReader(const std::string &path);
    ~IndexArchiveReader();
    int size();
    const Entry &entry(int index);
    int indexOf(const std::string &entryName);
    Index *openEntry(int index);

private:
    std::string m_path;
    std::vector<Entry*> m_entries;
    std::map<std::string, int> m_entryMap;
};

} // namespace indexdb

#endif // INDEXDB_INDEXARCHIVE_H
