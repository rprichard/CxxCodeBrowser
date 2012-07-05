#ifndef NAV_SOURCE_H
#define NAV_SOURCE_H

#include <QString>
#include <QStringList>

namespace Nav {

class Source
{
public:
    QString path;
    QStringList includes;
    QStringList defines;
    QStringList extraArgs;
};

} // namespace Nav

#endif // NAV_SOURCE_H
