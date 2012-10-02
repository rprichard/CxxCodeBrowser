#ifndef INDEXDB_INDEXARCHIVEBUILDER_H
#define INDEXDB_INDEXARCHIVEBUILDER_H

#include <map>
#include <string>

namespace indexdb {

class Index;

class IndexArchiveBuilder
{
public:
    ~IndexArchiveBuilder();
    void insert(const std::string &entryName, Index *index);
    Index *lookup(const std::string &entryName);
    void finalize();
    void write(const std::string &path, bool compressed=false);

private:
    std::map<std::string, Index*> m_indices;
};

} // namespace indexdb

#endif // INDEXDB_INDEXARCHIVEBUILDER_H
