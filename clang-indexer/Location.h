#ifndef INDEXER_LOCATION_H
#define INDEXER_LOCATION_H

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <sstream>
#include <string>

namespace indexer {

struct Location {
    const char *filename;
    unsigned int line;
    int column;

    std::string toString() const {
        std::stringstream ss;
        ss << filename << ":" << line << ":" << column;
        return ss.str();
    }
};

Location convertLocation(
        clang::SourceManager *pSM,
        clang::SourceLocation loc);

} // namespace indexer

#endif // INDEXER_LOCATION_H
