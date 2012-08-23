#include "File.h"
#include <QString>
#include <QFile>

namespace Nav {

File::File(const QString &path, const QString &content) :
    m_path(path), m_content(content)
{
}

File *File::readFile(const QString &path)
{
    QFile qfile(path);
    if (!qfile.open(QFile::ReadOnly))
        return NULL;
    File *file = new File(path, qfile.readAll());
    qfile.close();
    return file;
}

} // namespace Nav
