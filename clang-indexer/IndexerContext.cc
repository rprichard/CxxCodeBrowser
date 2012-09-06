#include "IndexerContext.h"

#include <clang/Basic/SourceManager.h>

namespace indexer {

IndexerContext::IndexerContext(
        clang::SourceManager &sourceManager,
        indexdb::Index &index) :
    m_sourceManager(sourceManager),
    m_indexBuilder(index),
    m_locationConverter(sourceManager, m_indexBuilder)
{
}

} // namespace indexer
