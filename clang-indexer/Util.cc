#include "Util.h"

namespace indexer {

// The POSIX basename can modify its path, which is inconvenient.
const char *const_basename(const char *path)
{
    const char *ret = path;
    while (true) {
        char ch = *path;
        if (ch == '\0') {
            break;
        } else if (ch == '/') {
            ret = path + 1;
        }
#ifdef _WIN32
        // Not tested
        else if (ch == '\\' || ch == ':') {
            ret = path + 1;
        }
#endif
        path++;
    }
    return ret;
}

} // namespace indexer
