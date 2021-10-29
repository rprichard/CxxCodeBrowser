#include "../shared_headers/host.h"

// All UNIX systems should have fseeko, which uses an off_t argument instead of
// a long argument.  It appears that Linux defines off_t to be long by default,
// which is 32-bit on 32-bit machines.  The _FILE_OFFSET_BITS macro should fix
// that.  Other OS's (like FreeBSD and MacOSX) always define off_t to 64-bits.
#ifdef __linux__
#define _FILE_OFFSET_BITS 64
#endif

#if defined(CXXCODEBROWSER_UNIX)
#include <unistd.h>
#elif defined(_WIN32)
#include <io.h>
#endif

#include <stdio.h>
#include <stdint.h>
#include "FileIo64BitSupport.h"

namespace indexdb {

// whence is one of SEEK_{CUR,END,SET}, just as with fseek.
int Seek64(FILE *fp, uint64_t offset, int whence)
{
#if defined(CXXCODEBROWSER_UNIX)
    return fseeko(fp, offset, whence);
#elif defined(_WIN32) && _MSC_VER >= 1400
    return _fseeki64(fp, offset, whence);
#else
#warning "No 64-bit fseek -- files larger than 2GB might break."
    return fseek(fp, offset, whence);
#endif
}

uint64_t Tell64(FILE *fp)
{
#if defined(CXXCODEBROWSER_UNIX)
    return ftello(fp);
#elif defined(_WIN32) && _MSC_VER >= 1400
    return _ftelli64(fp);
#else
#warning "No 64-bit ftell -- files larger than 2GB might break."
    return ftell(fp);
#endif
}

uint64_t LSeek64(int fd, uint64_t offset, int whence)
{
#if defined(CXXCODEBROWSER_UNIX)
    return lseek(fd, offset, whence);
#elif defined(_WIN32)
    return _lseeki64(fd, offset, whence);
#else
#error "Not implemented"
#endif
}

} // namespace indexdb
