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

    QString content() {
        ensureLoaded();
        return m_content;
    }

    int lineCount() {
        ensureLoaded();
        return m_lines.size();
    }

    // 0-based line number
    int lineStart(int line) {
        ensureLoaded();
        assert(line < lineCount());
        return m_lines[line].first;
    }

    // 0-based line number
    int lineLength(int line) {
        ensureLoaded();
        assert(line < lineCount());
        return m_lines[line].second;
    }

    // 0-based line number
    QStringRef lineContent(int line) {
        ensureLoaded();
        return QStringRef(&m_content, lineStart(line), lineLength(line));
    }

private:
    void loadFile();

    void ensureLoaded() {
        if (!m_loaded)
            loadFile();
    }

    QString m_path;
    bool m_loaded;
    QString m_content;
    std::vector<std::pair<int, int> > m_lines;
};

} // namespace Nav

#endif // NAV_FILE_H
