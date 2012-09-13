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

} // namespace Nav

#endif // NAV_REF_H
