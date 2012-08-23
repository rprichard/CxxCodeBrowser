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

    if (!fileMap.contains(path)) {
        File *f = File::readFile(path);
        if (f == NULL)
            f = new File(path, "Error: no such file");
        fileMap[path] = f;
    }
    return *fileMap[path];
}

} // namespace Nav
