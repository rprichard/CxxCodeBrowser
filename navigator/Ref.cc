#include "Ref.h"

#include "File.h"

namespace Nav {

bool operator<(const Ref &x, const Ref &y)
{
#define COMPARE(EXPR)       \
    do {                    \
        auto tmp = (EXPR);  \
        if (tmp < 0)        \
            return true;    \
        else if (tmp > 0)   \
            return false;   \
    } while(0)

    COMPARE(static_cast<int64_t>(x.m_symbolID) -
            static_cast<int64_t>(y.m_symbolID));
    COMPARE(static_cast<int64_t>(x.m_fileID) -
            static_cast<int64_t>(y.m_fileID));
    COMPARE(x.line() - y.line());
    COMPARE(x.column() - y.column());
    COMPARE(x.endColumn() - y.endColumn());
    COMPARE(static_cast<int64_t>(x.m_kindID) -
            static_cast<int64_t>(y.m_kindID));
#undef COMPARE

    return false;
}

} // namespace Nav
