#ifndef INDEXER_TUINDEXER_H
#define INDEXER_TUINDEXER_H

#include <string>
#include <vector>

namespace indexdb {
    class IndexArchiveBuilder;
}

namespace indexer {

void indexTranslationUnit(
        const std::vector<std::string> &argv,
        indexdb::IndexArchiveBuilder &archive);

} // namespace indexer

#endif // INDEXER_TUINDEXER_H
