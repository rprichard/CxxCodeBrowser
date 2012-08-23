#ifndef NAV_REF_H
#define NAV_REF_H

#include <QString>

namespace Nav {

class File;

struct Ref
{
    QString symbol;
    File *file;
    int line;
    int column;
    QString kind;
};

} // namespace Nav

#endif // NAV_REF_H
