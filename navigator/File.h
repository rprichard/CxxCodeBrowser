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
    QString path() { return m_path; }
    QString content() { return m_content; }

private:
    QString m_path;
    QString m_content;
};

} // namespace Nav

#endif // NAV_FILE_H
