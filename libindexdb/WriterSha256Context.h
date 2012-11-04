#ifndef INDEXDB_WRITERSHA256CONTEXT_H
#define INDEXDB_WRITERSHA256CONTEXT_H

#include <sha2.h>

namespace indexdb {

struct WriterSha256Context {
    sha256_ctx ctx;
};

} // namespace indexdb

#endif // INDEXDB_WRITERSHA256CONTEXT_H
