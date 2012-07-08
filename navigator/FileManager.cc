#include "FileManager.h"
#include "File.h"
#include <QFile>

namespace Nav {

FileManager::FileManager()
{
}

FileManager::~FileManager()
{
    foreach (File *value, fileMap) {
        delete value;
    }
}

File *FileManager::file(const QString &path)
{
    if (!fileMap.contains(path)) {
        File *f = File::readFile(path);
        if (f == NULL)
            return NULL;
        fileMap[path] = f;
    }
    return fileMap[path];
}

} // namespace Nav
