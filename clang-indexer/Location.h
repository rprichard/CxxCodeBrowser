#ifndef INDEXER_LOCATION_H
#define INDEXER_LOCATION_H

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <sstream>
#include <string>
#include <unordered_map>

#include "../libindexdb/IndexDb.h"

namespace indexer {

class IndexBuilder;

struct Location {
    indexdb::ID fileID;
    unsigned int line;
    int column;

    std::string toString(IndexBuilder &builder) const;
};

struct FileIDHash {
    size_t operator()(clang::FileID fileID) const {
        return fileID.getHashValue();
    }
};

class LocationConverter {
public:
    LocationConverter(clang::SourceManager &sourceManager, IndexBuilder &builder);
    Location convert(clang::SourceLocation loc);
private:
    clang::SourceManager &m_sourceManager;
    IndexBuilder &m_builder;
    std::unordered_map<clang::FileID, indexdb::ID, FileIDHash> m_realpathCache;
};

} // namespace indexer

#endif // INDEXER_LOCATION_H
