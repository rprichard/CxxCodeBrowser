#ifndef NAV_FILE_H
#define NAV_FILE_H

#include <QList>
#include <QString>
#include <cassert>
#include <string>
#include <utility>
#include <vector>

#include "FolderItem.h"
#include "StringRef.h"

namespace Nav {

class File : public FolderItem
{
    friend class FileManager;

private:
    File(Folder *parent, const QString &path);

public:
    Folder *parent() { return m_parent; }
    bool isFolder() { return false; }
    File *asFile() { return this; }
    QString path();
    QString title();

    const std::string &content() {
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
    StringRef lineContent(int line) {
        ensureLoaded();
        return StringRef(m_content.c_str() + lineStart(line), lineLength(line));
    }

private:
    void loadFile();

    void ensureLoaded() {
        if (!m_loaded)
            loadFile();
    }

    Folder *m_parent;
    QString m_path;
    bool m_loaded;
    std::string m_content;
    std::vector<std::pair<int, int> > m_lines;
};

} // namespace Nav

#endif // NAV_FILE_H
