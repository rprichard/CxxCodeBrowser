#ifndef INDEXER_INDEXERCONTEXT_H
#define INDEXER_INDEXERCONTEXT_H

#include "Location.h"
#include "IndexBuilder.h"

namespace clang {
    class Preprocessor;
    class SourceManager;
}

namespace indexdb {
    class Index;
}

namespace indexer {

class IndexerContext
{
public:
    IndexerContext(
            clang::SourceManager &sourceManager,
            clang::Preprocessor &preprocessor,
            indexdb::Index &index);
    clang::SourceManager &sourceManager() { return m_sourceManager; }
    clang::Preprocessor &preprocessor() { return m_preprocessor; }
    IndexBuilder &indexBuilder() { return m_indexBuilder; }
    LocationConverter &locationConverter() { return m_locationConverter; }

private:
    clang::SourceManager &m_sourceManager;
    clang::Preprocessor &m_preprocessor;
    IndexBuilder m_indexBuilder;
    LocationConverter m_locationConverter;
};

} // namespace indexer

#endif // INDEXER_INDEXERCONTEXT_H
