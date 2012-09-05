#include "IndexBuilder.h"

#include "../libindexdb/IndexDb.h"

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

IndexBuilder::IndexBuilder(indexdb::Index *index) : m_index(index)
{
    m_pathStringTable = m_index->stringTable("path");
    m_kindStringTable = m_index->stringTable("kind");
    m_usrStringTable = m_index->stringTable("usr");
    m_refTable = m_index->table("ref");
    m_locTable = m_index->table("loc");
}

} // namespace indexer
