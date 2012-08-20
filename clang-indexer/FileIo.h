#ifndef INDEXDB_FILEIO_H
#define INDEXDB_FILEIO_H

#include <stdint.h>

namespace indexdb {

const uint32_t kPageSize = 4096;

void padFile(int fd);

inline uint32_t roundToPage(uint32_t size)
{
    return (size + kPageSize - 1) & -kPageSize;
}

uint32_t tell(int fd);

} // namespace indexdb

#endif // INDEXDB_FILEIO_H
