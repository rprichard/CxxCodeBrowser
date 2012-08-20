#include "FileIo.h"
#include <cassert>

// UNIX headers
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace indexdb {

void padFile(int fd)
{
    static char zero[kPageSize];
    int extra = lseek(fd, 0, SEEK_CUR) & (kPageSize - 1);
    if (extra != 0) {
        ssize_t result = write(fd, zero, kPageSize - extra);
        assert(result == kPageSize - extra);
    }
}

uint32_t tell(int fd)
{
    return lseek(fd, 0, SEEK_CUR);
}

} // namespace indexdb
