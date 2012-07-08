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
    // HACK
    if (path == "<built-in>" || path.isEmpty())
        return NULL;

    if (!fileMap.contains(path)) {
        File *f = File::readFile(path);
        if (f == NULL)
            return NULL;
        fileMap[path] = f;
    }
    return fileMap[path];
}

} // namespace Nav
