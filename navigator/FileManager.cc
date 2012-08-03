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

void FileManager::addBuiltinFile(File *builtin)
{
    assert(!fileMap.contains(builtin->path));
    fileMap[builtin->path] = builtin;
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
