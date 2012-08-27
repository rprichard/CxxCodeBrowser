#include "File.h"
#include <QString>
#include <QFile>

namespace Nav {

File::File(const QString &path) :
    m_path(path), m_loaded(false)
{
}

QString File::path()
{
    return m_path;
}

void File::ensureLoaded()
{
    if (m_loaded)
        return;

    QFile qfile(m_path);
    if (!qfile.open(QFile::ReadOnly)) {
        m_content = "Error: cannot open " + m_path;
    } else {
        m_content = qfile.readAll();
        QChar *data = m_content.data();
        int lineStart = 0;
        for (int i = 0; i < m_content.size(); ) {
            if (data[i] == '\r' || data[i] == '\n') {
                m_lines.push_back(std::make_pair(lineStart, i));
                if (data[i] == '\r' && data[i + 1] == '\n')
                    i++;
                i++;
                lineStart = i;
            } else {
                i++;
            }
        }
        if (m_content.size() > lineStart)
            m_lines.push_back(std::make_pair(lineStart, m_content.size()));
    }
}

QString File::content()
{
    ensureLoaded();
    return m_content;
}

int File::lineCount()
{
    ensureLoaded();
    return m_lines.size();
}

// 1-based line index
QStringRef File::lineContent(int line)
{
    Q_ASSERT(line >= 1 && line <= m_lines.size());
    auto range = m_lines[line - 1];
    return m_content.midRef(range.first, range.second - range.first);
}

} // namespace Nav
