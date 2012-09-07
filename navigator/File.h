#ifndef NAV_FILE_H
#define NAV_FILE_H

#include "Misc.h"
#include <QList>
#include <QString>
#include <cassert>
#include <string>
#include <vector>
#include <utility>

namespace Nav {

class File
{
public:
    File(const QString &path);
    QString path();
    QString content();
    int lineCount();
    int lineStart(int line);
    int lineLength(int line);
    QStringRef lineContent(int line);

private:
    void ensureLoaded();

    QString m_path;
    bool m_loaded;
    QString m_content;
    std::vector<int> m_lines;
};

} // namespace Nav

#endif // NAV_FILE_H
