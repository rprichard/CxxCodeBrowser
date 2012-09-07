#include "File.h"
#include <QString>
#include <QFile>
#include <cstdlib>
#include <cstring>

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
    }

    QChar *data = m_content.data();
    const QChar charCR = '\r';
    const QChar charNL = '\n';

    int lineStart = 0;

    for (int i = 0; ; ) {
        if (data[i] == charCR || data[i] == charNL) {
            m_lines.push_back(std::make_pair(lineStart, i - lineStart));
            if (data[i] == charCR && data[i + 1] == charNL)
                i++;
            i++;
            lineStart = i;
        } else if (data[i].isNull()) {
            if (i > lineStart)
                m_lines.push_back(std::make_pair(lineStart, i - lineStart));
            break;
        } else {
            i++;
        }
    }

    m_loaded = true;
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

// 0-based line number
int File::lineStart(int line)
{
    ensureLoaded();
    assert(line < lineCount());
    return m_lines[line].first;
}

// 0-based line number
int File::lineLength(int line)
{
    ensureLoaded();
    assert(line < lineCount());
    return m_lines[line].second;
}

// 0-based line number
QStringRef File::lineContent(int line)
{
    ensureLoaded();
    return QStringRef(&m_content, lineStart(line), lineLength(line));
}

} // namespace Nav
