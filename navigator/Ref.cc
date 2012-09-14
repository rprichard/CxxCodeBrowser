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

    COMPARE(x.symbol.compare(y.symbol));
    if (x.file != y.file) {
        if (x.file == NULL && y.file != NULL)
            return true;
        if (x.file != NULL && y.file == NULL)
            return false;
        COMPARE(x.file->path().compare(y.file->path()));
    }

    COMPARE(x.line - y.line);
    COMPARE(x.column - y.column);
    COMPARE(x.kind.compare(y.kind));
#undef COMPARE

    return false;
}

} // namespace Nav
