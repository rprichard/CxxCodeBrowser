#ifndef NAV_SOURCEWIDGET_H
#define NAV_SOURCEWIDGET_H

#include <QList>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <utility>

#include "File.h"
#include "CXXSyntaxHighlighter.h"

namespace Nav {

class File;
class Project;
class SourceWidget;
class SourceWidgetLineArea;


///////////////////////////////////////////////////////////////////////////////
// FileLocation / FileRange

typedef uint32_t FileOffset;

// Indices are 0-based.
//
// XXX: Should they be 1-based?  Should File's indices be 1-based?)
//
// States:
//  - null is (-1,-1)
//  - doesPointAtChar is (n,m) where n in [0..lineCount) and m in [0..lineLength(n)).
//  - eol is (n,m) where n in [0..lineCount) and m == lineLength(n).
//  - eof is (n,0) where n == lineCount
struct FileLocation {
    FileLocation() : line(-1), column(-1) {}
    FileLocation(int line, int column) : line(line), column(column) {}
    int line;
    int column;

    bool isNull() const { return line < 0 || column < 0; }
    bool doesPointAtChar(File &file) const {
        return line >= 0 && column >= 0 &&
                line < file.lineCount() &&
                column < file.lineLength(line);
    }

    FileOffset toOffset(File &file) const {
        if (line < 0)
            return ~0;
        else if (line < file.lineCount() && column <= file.lineLength(line))
            // XXX: This line is mostly OK...
            return file.lineStart(line) + column;
        else if (line == file.lineCount() && column == 0)
            // XXX: ...but does this line really make sense?  It's interesting
            // that the file content has single newline characters separating
            // the lines, which had previously been irrelevant but now matter
            // because they keep the EOL offset distinct from the offset of the
            // start of the next line.
            return file.content().size();
        else
            return ~0;
    }
};

inline bool operator<(const FileLocation &x, const FileLocation &y) {
    if (x.line < y.line)
        return true;
    else if (x.line > y.line)
        return false;
    else
        return x.column < y.column;
}

inline bool operator>(const FileLocation &x, const FileLocation &y) {
    if (x.line > y.line)
        return true;
    else if (x.line < y.line)
        return false;
    else
        return x.column > y.column;
}

inline bool operator==(const FileLocation &x, const FileLocation &y) {
    return x.line == y.line && x.column == y.column;
}

inline bool operator!=(const FileLocation &x, const FileLocation &y) { return !(x == y); }
inline bool operator>=(const FileLocation &x, const FileLocation &y) { return !(x < y); }
inline bool operator<=(const FileLocation &x, const FileLocation &y) { return !(x > y); }

struct FileRange {
    FileRange() {}
    FileRange(const FileLocation &start, const FileLocation &end) :
        start(start), end(end)
    {
    }
    FileLocation start;
    FileLocation end;
    bool isEmpty() { return start == end; }
};

inline bool operator==(const FileRange &x, const FileRange &y) {
    return x.start == y.start && x.end == y.end;
}

inline bool operator!=(const FileRange &x, const FileRange &y) { return !(x == y); }


///////////////////////////////////////////////////////////////////////////////
// SourceWidgetLineArea

// The line number area to the left of the source widget.
class SourceWidgetLineArea : public QWidget
{
    Q_OBJECT
public:
    SourceWidgetLineArea(const QMargins &margins, QWidget *parent = 0) :
        QWidget(parent), m_margins(margins), m_lineCount(0)
    {
    }

    QSize sizeHint() const;
    void paintEvent(QPaintEvent *event);
    int lineCount() const                   { return m_lineCount; }
    void setLineCount(int lineCount)        { m_lineCount = lineCount; updateGeometry(); update(); }
private:
    QMargins m_margins;
    int m_lineCount;
};


///////////////////////////////////////////////////////////////////////////////
// SourceWidgetView

// The widget showing the actual text, inside the source widget's viewport.
class SourceWidgetView : public QWidget
{
    Q_OBJECT
public:
    SourceWidgetView(const QMargins &margins, Project &project);
    void setFile(File *file);
    File *file() { return m_file; }
    FileLocation hitTest(QPoint pt);
    FileRange findWordAtLocation(const FileLocation &pt);
    FileRange findWordAtPoint(QPoint pt);
    void setSelection(const FileRange &fileRange) { m_selection = fileRange; update(); }
    QPoint locationToPoint(FileLocation loc);
    QSize sizeHint() const;
    QSize minimumSizeHint() const { return sizeHint(); }

signals:
    void goBack();
    void goForward();
    void areBackAndForwardEnabled(bool &backEnabled, bool &forwardEnabled);

private:
    void paintEvent(QPaintEvent *event);

    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);

private slots:
    void actionCrossReferences();

private:
    QMargins m_margins;
    Project &m_project;
    File *m_file;
    QVector<CXXSyntaxHighlighter::Kind> m_syntaxColoring;
    int m_maxLineLength;
    FileRange m_selection;
};


///////////////////////////////////////////////////////////////////////////////
// SourceWidget

class SourceWidget : public QScrollArea
{
    Q_OBJECT
public:
    explicit SourceWidget(Project &project, QWidget *parent = 0);
    void setFile(File *file);
    File *file() { return sourceWidgetView().file(); }
    void selectIdentifier(int line, int column);
    QPoint viewportOrigin();
    void setViewportOrigin(const QPoint &pt);

signals:
    void fileChanged(File *file);
    void goBack();
    void goForward();
    void areBackAndForwardEnabled(bool &backEnabled, bool &forwardEnabled);

private:
    SourceWidgetView &sourceWidgetView();
    void resizeEvent(QResizeEvent *event);
    void keyPressEvent(QKeyEvent *event);

private slots:
    void layoutSourceWidget(void);

private:
    SourceWidgetLineArea *m_lineArea;
    Project &m_project;
    friend class SourceWidgetLineArea;
};

} // namespace Nav

#endif // NAV_SOURCEWIDGET_H
