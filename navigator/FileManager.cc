#include "FileManager.h"
#include "File.h"
#include <QFile>
#include <assert.h>

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

File &FileManager::file(const QString &path)
{
    assert(!path.isEmpty());
    if (!fileMap.contains(path))
        fileMap[path] = new File(path);
    return *fileMap[path];
}

} // namespace Nav
