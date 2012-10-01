#ifndef INDEXER_LOCATION_H
#define INDEXER_LOCATION_H

#include "../libindexdb/IndexDb.h"

namespace indexer {

struct Location {
    indexdb::ID fileID;
    unsigned int line;
    int column;
};

} // namespace indexer

#endif // INDEXER_LOCATION_H
