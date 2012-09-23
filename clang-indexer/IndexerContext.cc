#include "IndexerContext.h"

#include <clang/Basic/SourceManager.h>

namespace indexer {

IndexerContext::IndexerContext(
        clang::SourceManager &sourceManager,
        clang::Preprocessor &preprocessor,
        indexdb::Index &index) :
    m_sourceManager(sourceManager),
    m_preprocessor(preprocessor),
    m_indexBuilder(index),
    m_locationConverter(sourceManager, m_indexBuilder)
{
}

} // namespace indexer
