#ifndef NAV_REF_H
#define NAV_REF_H

#include <QString>

namespace Nav {

class File;

struct Ref
{
    QString symbol;
    File *file;
    int line;           // 1-based
    int column;         // 1-based
    QString kind;
};

bool operator<(const Ref &x, const Ref &y);

inline bool operator==(const Ref &x, const Ref &y)
{
    return x.symbol == y.symbol &&
            x.file == y.file &&
            x.line == y.line &&
            x.column == y.column &&
            x.kind == y.kind;
}

inline bool operator!=(const Ref &x, const Ref &y)
{
    return !(x == y);
}

} // namespace Nav

#endif // NAV_REF_H
