#include "IndexBuilder.h"

#include "../libindexdb/IndexDb.h"
#include "Location.h"

namespace indexer {

indexdb::Index *newIndex()
{
    indexdb::Index *index = new indexdb::Index;

    index->addStringTable("path");
    index->addStringTable("kind");
    index->addStringTable("usr");

    std::vector<std::string> refColumns;
    refColumns.push_back("usr");
    refColumns.push_back("path");
    refColumns.push_back(""); // line
    refColumns.push_back(""); // column
    refColumns.push_back("kind");
    index->addTable("ref", refColumns);

    std::vector<std::string> locColumns;
    locColumns.push_back("path");
    locColumns.push_back(""); // line
    locColumns.push_back(""); // column
    locColumns.push_back("usr");
    index->addTable("loc", locColumns);

    return index;
}

IndexBuilder::IndexBuilder(indexdb::Index &index) : m_index(index)
{
    m_pathStringTable = m_index.stringTable("path");
    m_kindStringTable = m_index.stringTable("kind");
    m_usrStringTable = m_index.stringTable("usr");
    m_refTable = m_index.table("ref");
    m_locTable = m_index.table("loc");
}

void IndexBuilder::recordRef(const char *usr, const Location &loc, const char *kind)
{
    indexdb::ID kindID = m_kindStringTable->insert(kind);
    indexdb::ID usrID = m_usrStringTable->insert(usr);

    {
        indexdb::Row refRow(5);
        refRow[0] = usrID;
        refRow[1] = loc.fileID;
        refRow[2] = loc.line;
        refRow[3] = loc.column;
        refRow[4] = kindID;
        m_refTable->add(refRow);
    }

    {
        indexdb::Row locRow(4);
        locRow[0] = loc.fileID;
        locRow[1] = loc.line;
        locRow[2] = loc.column;
        locRow[3] = usrID;
        m_locTable->add(locRow);
    }
}

} // namespace indexer
