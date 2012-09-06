#ifndef INDEXER_TUINDEXER_H
#define INDEXER_TUINDEXER_H

#include <string>
#include <vector>

namespace indexdb {
    class Index;
}

namespace indexer {

void indexTranslationUnit(const std::vector<std::string> &argv, indexdb::Index &index);

} // namespace indexer

#endif // INDEXER_TUINDEXER_H
