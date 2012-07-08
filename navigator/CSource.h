#ifndef NAV_CSOURCE_H
#define NAV_CSOURCE_H

#include <QString>
#include <QStringList>

namespace Nav {

//
// A C/C++ source file (not header).
//
// A CSource object includes all the compiler options needed to parse the
// file itself.  There can be multiple CSource objects for a single file.
//
class CSource
{
public:
    QString path;
    QStringList includes;
    QStringList defines;
    QStringList extraArgs;
};

} // namespace Nav

#endif // NAV_CSOURCE_H
