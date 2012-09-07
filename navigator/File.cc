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
        // Read the file and canonicalize CRLF and CR line endings to LF.
        char *buf = new char[qfile.size() + 1];
        qfile.read(buf, qfile.size());
        buf[qfile.size()] = '\0';
        char *d = buf, *s = buf;
        while (*s != '\0') {
            if (*s == '\r') {
                *(d++) = '\n';
                ++s;
                if (*s == '\n')
                    ++s;
            } else {
                *(d++) = *(s++);
            }
        }
        *(d++) = '\0';
        m_content = buf;
        delete [] buf;
    }

    // Identify where each line begins.
    QChar *data = m_content.data();
    const QChar charNL = '\n';
    int lineStart = 0;
    for (int i = 0; ; i++) {
        if (data[i] == charNL) {
            m_lines.push_back(lineStart);
            lineStart = i + 1;
        } else if (data[i].isNull()) {
            if (i > lineStart)
                m_lines.push_back(lineStart);
            break;
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
    return m_lines[line];
}

// 0-based line number
int File::lineLength(int line)
{
    ensureLoaded();
    assert(line < lineCount());

    int startPos = lineStart(line);
    int endPos;
    if (line + 1 < lineCount())
        endPos = lineStart(line + 1);
    else
        endPos = m_content.size();

    if (startPos < endPos && m_content[endPos - 1] == QChar('\n'))
        --endPos;

    return endPos - startPos;
}

// 0-based line number
QStringRef File::lineContent(int line)
{
    ensureLoaded();
    return QStringRef(&m_content, lineStart(line), lineLength(line));
}

} // namespace Nav
