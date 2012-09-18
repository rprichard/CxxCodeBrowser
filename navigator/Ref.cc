#include "Ref.h"

#include "File.h"

namespace Nav {

bool operator<(const Ref &x, const Ref &y)
{
    // TODO: This code could be a lot faster if the string tables for symbols,
    // paths, and kinds were in sorted order.

#define COMPARE(EXPR)       \
    do {                    \
        auto tmp = (EXPR);  \
        if (tmp < 0)        \
            return true;    \
        else if (tmp > 0)   \
            return false;   \
    } while(0)

    if (x.m_symbolID != y.m_symbolID)
        COMPARE(x.symbol().compare(y.symbol()));
    if (x.m_fileID != y.m_fileID)
        COMPARE(x.file().path().compare(y.file().path()));
    COMPARE(x.line() - y.line());
    COMPARE(x.column() - y.column());
    if (x.m_kindID != y.m_kindID)
        COMPARE(x.kind().compare(y.kind()));
#undef COMPARE

    return false;
}

} // namespace Nav
