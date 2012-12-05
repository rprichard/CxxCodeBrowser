#ifndef INDEXER_UTIL_H
#define INDEXER_UTIL_H

#include <cstdio>
#include <ctime>
#include <string>

namespace indexer {

const time_t kInvalidTime = static_cast<time_t>(-1);

#ifdef __unix__
#define EINTR_LOOP(expr)                        \
    ({                                          \
        decltype(expr) ret;                     \
        do {                                    \
            ret = (expr);                       \
        } while (ret == -1 && errno == EINTR);  \
        ret;                                    \
    })
#endif

const char *const_basename(const char *path);
char *portableRealPath(const char *path);
time_t getPathModTime(const std::string &path);
bool stringStartsWith(const std::string &str, const std::string &suffix);
bool stringEndsWith(const std::string &str, const std::string &suffix);
std::string readLine(FILE *fp, bool *isEof = NULL);

} // namespace indexer

#endif // INDEXER_UTIL_H
