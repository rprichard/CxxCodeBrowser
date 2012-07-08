#ifndef NAV_FILEMANAGER_H
#define NAV_FILEMANAGER_H

#include <QHash>
#include <QString>

namespace Nav {

class File;

class FileManager
{
public:
    FileManager();
    ~FileManager();
    File *file(const QString &path);

private:
    QHash<QString, File*> fileMap;
};

} // namespace Nav

#endif // NAV_FILEMANAGER_H
