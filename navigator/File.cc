#include "File.h"
#include <QString>
#include <QFile>

namespace Nav {

File *File::readFile(const QString &path)
{
    QFile qfile(path);
    if (!qfile.open(QFile::ReadOnly))
        return NULL;
    File *file = new File;
    file->path = path;
    file->content = qfile.readAll();
    qfile.close();
    return file;
}

File::File()
{
}

} // namespace Nav
