#ifndef INDEXER_LOCATION_H
#define INDEXER_LOCATION_H

#include "../libindexdb/IndexDb.h"

namespace indexer {

struct Location {
    indexdb::ID fileID;
    unsigned int line;  // 1-based
    int column;         // 1-based
};

} // namespace indexer

#endif // INDEXER_LOCATION_H
