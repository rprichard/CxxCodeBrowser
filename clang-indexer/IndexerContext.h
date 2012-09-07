#ifndef INDEXER_INDEXERCONTEXT_H
#define INDEXER_INDEXERCONTEXT_H

#include "Location.h"
#include "IndexBuilder.h"

namespace clang {
    class SourceManager;
}

namespace indexdb {
    class Index;
}

namespace indexer {

class IndexerContext
{
public:
    IndexerContext(clang::SourceManager &sourceManager, indexdb::Index &index);
    clang::SourceManager &sourceManager() { return m_sourceManager; }
    IndexBuilder &indexBuilder() { return m_indexBuilder; }
    LocationConverter &locationConverter() { return m_locationConverter; }

private:
    clang::SourceManager &m_sourceManager;
    IndexBuilder m_indexBuilder;
    LocationConverter m_locationConverter;
};

} // namespace indexer

#endif // INDEXER_INDEXERCONTEXT_H
