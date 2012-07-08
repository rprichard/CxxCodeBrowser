#ifndef NAV_FILE_H
#define NAV_FILE_H

#include "Misc.h"
#include <QString>

namespace Nav {

class File
{
public:
    static File *readFile(const QString &path);
    QString path;
    QString content;

private:
    File();
};

} // namespace Nav

#endif // NAV_FILE_H
