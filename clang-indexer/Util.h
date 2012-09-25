#ifndef INDEXER_UTIL_H
#define INDEXER_UTIL_H

#include <cstdio>
#include <string>

namespace indexer {

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
bool stringStartsWith(const std::string &str, const std::string &suffix);
bool stringEndsWith(const std::string &str, const std::string &suffix);
std::string readLine(FILE *fp, bool *isEof = NULL);

} // namespace indexer

#endif // INDEXER_UTIL_H
