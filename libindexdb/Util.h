#ifndef INDEXDB_UTIL_H
#define INDEXDB_UTIL_H

#include <stdint.h>

#ifdef __linux__
#include <byteswap.h>
#endif

#include "Endian.h"

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

namespace indexdb {

inline uint32_t byteSwap32(uint32_t val)
{
#ifdef __linux__
    return bswap_32(val);
#else
    return  ((val & 0xFF000000u) >> 24) |
            ((val & 0x00FF0000u) >> 8) |
            ((val & 0x0000FF00u) << 8) |
            ((val & 0x000000FFu) << 24);
#endif
}

inline uint32_t HostToLE32(uint32_t val)
{
#if INDEXDB_BYTE_ORDER == INDEXDB_BIG_ENDIAN
    return byteSwap32(val);
#elif INDEXDB_BYTE_ORDER == INDEXDB_LITTLE_ENDIAN
    return val;
#else
#error "Unrecognized INDEXDB_BYTE_ORDER value."
#endif
}

inline uint32_t LEToHost32(uint32_t val)
{
#if INDEXDB_BYTE_ORDER == INDEXDB_BIG_ENDIAN
    return byteSwap32(val);
#elif INDEXDB_BYTE_ORDER == INDEXDB_LITTLE_ENDIAN
    return val;
#else
#error "Unrecognized INDEXDB_BYTE_ORDER value."
#endif
}

inline uint32_t HostToBE32(uint32_t val)
{
#if INDEXDB_BYTE_ORDER == INDEXDB_BIG_ENDIAN
    return val;
#elif INDEXDB_BYTE_ORDER == INDEXDB_LITTLE_ENDIAN
    return byteSwap32(val);
#else
#error "Unrecognized INDEXDB_BYTE_ORDER value."
#endif
}

inline uint32_t BEToHost32(uint32_t val)
{
#if INDEXDB_BYTE_ORDER == INDEXDB_BIG_ENDIAN
    return val;
#elif INDEXDB_BYTE_ORDER == INDEXDB_LITTLE_ENDIAN
    return byteSwap32(val);
#else
#error "Unrecognized INDEXDB_BYTE_ORDER value."
#endif
}

} // namespace indexdb

#endif // INDEXDB_UTIL_H
