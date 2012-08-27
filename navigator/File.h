#ifndef NAV_FILE_H
#define NAV_FILE_H

#include "Misc.h"
#include <QList>
#include <QString>
#include <utility>

namespace Nav {

class File
{
public:
    File(const QString &path);
    QString path();
    QString content();
    int lineCount();
    QStringRef lineContent(int line);

private:
    void ensureLoaded();

    QString m_path;
    QString m_content;
    bool m_loaded;
    QList<std::pair<int, int> > m_lines;
};

} // namespace Nav

#endif // NAV_FILE_H
