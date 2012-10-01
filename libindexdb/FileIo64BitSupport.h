#ifndef INDEXDB_FILEIO64BITSUPPORT_H
#define INDEXDB_FILEIO64BITSUPPORT_H

#include <stdint.h>

namespace indexdb {

int Seek64(FILE *fp, uint64_t offset, int whence);
uint64_t Tell64(FILE *fp);
uint64_t LSeek64(int fd, uint64_t offset, int whence);

} // namespace indexdb

#endif // INDEXDB_FILEIO64BITSUPPORT_H
