#ifndef NAV_SOURCEWIDGET_H
#define NAV_SOURCEWIDGET_H

#include <QList>
#include <QPlainTextEdit>
#include <QPoint>
#include <QScrollArea>
#include <QTime>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "CXXSyntaxHighlighter.h"
#include "File.h"
#include "Regex.h"
#include "RegexMatchList.h"
#include "StringRef.h"

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

    FileLocation(File &file, FileOffset offset) {
        if (offset == file.content().size()) {
            line = file.lineCount();
            column = 0;
        } else {
            line = file.lineForOffset(offset);
            column = offset - file.lineStart(line);
        }
    }

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
    bool isEmpty() const { return start == end; }
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
    virtual ~SourceWidgetView();
    void setFile(File *file);
    File *file() { return m_file; }
    FileLocation hitTest(QPoint pt, bool roundToNearest=false);
    QPoint locationToPoint(FileLocation loc);
    QSize sizeHint() const;
    QSize minimumSizeHint() const { return sizeHint(); }
    const Regex &findRegex() { return m_findRegex; }
    void setFindRegex(const Regex &findRegex);
    const RegexMatchList &findMatches() const { return m_findMatches; }
    int selectedMatchIndex() const { return m_selectedMatchIndex; }
    void setSelectedMatchIndex(int index);

public slots:
    void copy();

public:
    void setSelection(const FileRange &fileRange);
private:
    void setHoverHighlight(const FileRange &fileRange);
    void updateRange(const FileRange &range);
    StringRef rangeText(const FileRange &range);

signals:
    void goBack();
    void goForward();
    void areBackAndForwardEnabled(bool &backEnabled, bool &forwardEnabled);
    void copyFilePath();
    void revealInSideBar();
    void pointSelected(QPoint point);
    void findMatchSelectionChanged(int index);
    void findMatchListChanged();

private:
    FileRange findRefAtLocation(const FileLocation &pt);
    FileRange findRefAtPoint(QPoint pt);
    std::set<std::string> findSymbolsAtRange(const FileRange &range);
    FileRange findWordAtLocation(FileLocation loc);
    void paintEvent(QPaintEvent *event);
    int lineTop(int line);
    void paintLine(
            QPainter &painter,
            int line,
            const QRect &paintRect,
            RegexMatchList::iterator &findMatch);

    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void navMouseSingleDownEvent(QMouseEvent *event);
    void navMouseDoubleDownEvent(QMouseEvent *event);
    void navMouseTripleDownEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void moveEvent(QMoveEvent *event);
    bool event(QEvent *event);
    void updateSelectionAndHover();
    void updateSelectionAndHover(QPoint pos);
    void mouseReleaseEvent(QMouseEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
    void updateFindMatches();
    FileRange matchFileRange(int index);

private slots:
    void actionCrossReferences();

private:
    QMargins m_margins;
    Project &m_project;
    File *m_file;
    std::vector<Qt::GlobalColor> m_syntaxColoring;
    int m_maxLineLength;
    QPoint m_tripleClickPoint;
    QTime m_tripleClickTime;
    bool m_mouseHoveringInWidget;
    enum SelectingMode { SM_Inactive, SM_Ref, SM_Char, SM_Word, SM_Line }
        m_selectingMode;
    FileRange m_selectedRange;
    QPoint m_selectingAnchor;
    FileRange m_hoverHighlightRange;
    Regex m_findRegex;
    RegexMatchList m_findMatches;
    int m_selectedMatchIndex;
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
    void selectIdentifier(int line, int column, int endColumn);
    QPoint viewportOrigin();
    void setViewportOrigin(const QPoint &pt);

public slots:
    void copy();

signals:
    void fileChanged(File *file);
    void goBack();
    void goForward();
    void areBackAndForwardEnabled(bool &backEnabled, bool &forwardEnabled);
    void copyFilePath();
    void revealInSideBar();
    void findMatchSelectionChanged(int index);
    void findMatchListChanged();

private:
    SourceWidgetView &sourceWidgetView();
    void resizeEvent(QResizeEvent *event);
    void keyPressEvent(QKeyEvent *event);

private slots:
    void layoutSourceWidget(void);
    void viewPointSelected(QPoint point);

    // Methods for the "find" functionality.
public:
    void recordFindStart();
    void endFind();
    void setFindRegex(const Regex &findRegex, bool advanceToMatch);
    int matchCount();
    int selectedMatchIndex();
public slots:
    void selectNextMatch();
    void selectPreviousMatch();
    void setSelectedMatchIndex(int index);
private:
    int bestMatchIndex(int previousMatchOffset);
    void ensureSelectedMatchVisible();

private:
    QWidget *m_lineAreaViewport;
    SourceWidgetLineArea *m_lineArea;
    Project &m_project;

    // The view origin and top-left file offset when the user began searching.
    // Clearing the search returns the view to this origin.  Whenever the
    // search regex changes, the SourceWidget uses this file offset to decide
    // which match to select.
    QPoint m_findStartOrigin;
    int m_findStartOffset;

    friend class SourceWidgetLineArea;
};

} // namespace Nav

#endif // NAV_SOURCEWIDGET_H
