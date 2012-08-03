#ifndef NAV_FILE_H
#define NAV_FILE_H

#include "Misc.h"
#include <QString>

namespace Nav {

class File
{
public:
    File(const QString &path, const QString &content);
    static File *readFile(const QString &path);
    QString path;
    QString content;
};

} // namespace Nav

#endif // NAV_FILE_H
