#include "Ref.h"
#include <QHash>

namespace Nav {

unsigned int qHash(const Nav::Ref &ref)
{
    unsigned result = 0;
    result = result * 31 + ::qHash(ref.symbol);
    result = result * 31 + ::qHash(ref.file);
    result = result * 31 + ::qHash(ref.line);
    result = result * 31 + ::qHash(ref.column);
    result = result * 31 + ::qHash(ref.kind);
    return result;
}

bool operator==(const Nav::Ref &r1, const Nav::Ref &r2)
{
    return r1.symbol == r2.symbol &&
            r1.file == r2.file &&
            r1.line == r2.line &&
            r1.column == r2.column &&
            r1.kind == r2.kind;
}

} // namespace Nav
