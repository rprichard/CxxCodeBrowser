#ifndef INDEXDB_ENDIAN_H
#define INDEXDB_ENDIAN_H

//
// Attempt to determine the byte-order from the compiler macros.
//
// It defines INDEXDB_BYTE_ORDER to one of INDEXDB_{LITTLE,BIG}_ENDIAN.
//
// This code may need to be adjusted for the target system.  It will probably
// always work with GCC and Clang.
//

#define INDEXDB_LITTLE_ENDIAN 1234
#define INDEXDB_BIG_ENDIAN    4321

#if defined(__BYTE_ORDER__) && \
        defined(__ORDER_LITTLE_ENDIAN__) && \
        defined(__ORDER_BIG_ENDIAN__)
    // This branch should handle GCC 4.6 and later.
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define INDEXDB_BYTE_ORDER INDEXDB_LITTLE_ENDIAN
    #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        #define INDEXDB_BYTE_ORDER INDEXDB_BIG_ENDIAN
    #else
        #error "Unhandled __BYTE_ORDER__ value"
    #endif
#elif __i386__ || __x86_64__ || defined(_M_IX86) || defined(_M_X64)
    // Detect various compiler-specific macros for x86/x64, which is always
    // little-endian.
    #define INDEXDB_BYTE_ORDER INDEXDB_LITTLE_ENDIAN
#elif __LITTLE_ENDIAN__ && !__BIG_ENDIAN__
    // The __{LITTLE,BIG}_ENDIAN__ macros are defined to 1 with Clang.
    #define INDEXDB_BYTE_ORDER INDEXDB_LITTLE_ENDIAN
#elif __BIG_ENDIAN__ && !__LITTLE_ENDIAN__
    #define INDEXDB_BYTE_ORDER INDEXDB_BIG_ENDIAN
#else
    #error "Could not determine byte-endian."
#endif

#endif // INDEXDB_ENDIAN_H
