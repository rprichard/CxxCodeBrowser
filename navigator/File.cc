#include "File.h"

#include <QString>
#include <QFile>
#include <QFileInfo>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>

namespace Nav {

File::File(Folder *parent, const QString &path) :
    m_parent(parent), m_path(path), m_loaded(false)
{
}

QString File::path()
{
    return m_path;
}

QString File::title()
{
    QFileInfo fi(m_path);
    return fi.fileName();
}

// 0-based line number.  The offset must be less than the content size.  The
// result will be <= the line count.
int File::lineForOffset(int offset)
{
    ensureLoaded();
    assert(offset < static_cast<int>(m_content.size()));
    int line1 = 0;
    int line2 = lineCount() - 1;
    while (line1 < line2) {
        int midLine = line1 + (line2 - line1) / 2 + 1;
        assert(midLine > line1 && midLine <= line2);
        if (offset < m_lines[midLine].first)
            line2 = midLine - 1;
        else
            line1 = midLine;
    }
    return line1;
}

void File::loadFile()
{
    QFile qfile(m_path);
    if (!qfile.open(QFile::ReadOnly)) {
        m_content = "Error: cannot open " + m_path.toStdString();
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
        m_content = std::string(buf, d - buf);
        delete [] buf;
    }

    // Identify where each line begins.
    const char *data = m_content.c_str();
    int lineStart = 0;
    for (int i = 0; ; i++) {
        if (data[i] == '\n') {
            m_lines.push_back(std::make_pair(lineStart, i - lineStart));
            lineStart = i + 1;
        } else if (data[i] == '\0') {
            if (i > lineStart)
                m_lines.push_back(std::make_pair(lineStart, i - lineStart));
            break;
        }
    }

    m_loaded = true;
}

} // namespace Nav
