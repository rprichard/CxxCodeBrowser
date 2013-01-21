#ifndef NAV_SOURCEWIDGET_H
#define NAV_SOURCEWIDGET_H

#include <QAbstractScrollArea>
#include <QColor>
#include <QContextMenuEvent>
#include <QEvent>
#include <QKeyEvent>
#include <QList>
#include <QMargins>
#include <QMouseEvent>
#include <QMoveEvent>
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QResizeEvent>
#include <QTime>
#include <QWidget>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "../libindexdb/IndexDb.h"
#include "CXXSyntaxHighlighter.h"
#include "File.h"
#include "Regex.h"
#include "RegexMatchList.h"
#include "StringRef.h"

namespace Nav {

class File;
class Project;
class Ref;
class SourceWidget;
class SourceWidgetLineArea;


// Work around QTBUG-29220 by defeating QMacScrollOptimization.
#ifndef NAV_MACSCROLLOPTIMIZATION_HACK
#if defined(__APPLE__) && QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#define NAV_MACSCROLLOPTIMIZATION_HACK 1
#else
#define NAV_MACSCROLLOPTIMIZATION_HACK 0
#endif
#endif


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

    QString toString() const {
        return QString("(%0, %1)").arg(line).arg(column);
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

    QString toString() const {
        return QString("[%0-%1]").arg(start.toString()).arg(end.toString());
    }
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
        setAutoFillBackground(true);
        setBackgroundRole(QPalette::Window);
    }

    QSize sizeHint() const;
    void paintEvent(QPaintEvent *event);
    int lineCount() const                   { return m_lineCount; }
    void setLineCount(int lineCount)        { m_lineCount = lineCount; updateGeometry(); update(); }
    void setViewportOrigin(QPoint pt);
private:
    QPoint m_viewportOrigin;
    QMargins m_margins;
    int m_lineCount;
};


///////////////////////////////////////////////////////////////////////////////
// SourceWidgetTextPalette

class SourceWidgetTextPalette {
public:
    enum class Color : uint8_t {
        transparent = 0,
        defaultText = 1,
        highlightedText = 2,
        specialColorCount = 3
    };

    SourceWidgetTextPalette(Project &project);
    void setDefaultTextColor(const QColor &color);
    void setHighlightedTextColor(const QColor &color);
    inline Color colorForSyntaxKind(CXXSyntaxHighlighter::Kind kind) const;
    inline Color colorForRef(const Ref &ref) const;
    inline const QPen &pen(Color color) const;

private:
    void setSyntaxColor(CXXSyntaxHighlighter::Kind kind, const QColor &color);
    void setSymbolTypeColor(const char *symbolType, const QColor &color);
    Color registerColor(const QColor &color);

    Project &m_project;
    std::unordered_map<indexdb::ID, Color> m_symbolTypeColor;
    std::vector<Color> m_syntaxColor;
    std::vector<QPen> m_pens;
};


///////////////////////////////////////////////////////////////////////////////
// SourceWidgetView

// The widget showing the actual text, inside the source widget's viewport.
class SourceWidgetView : public QWidget
{
    Q_OBJECT
public:
    SourceWidgetView(
            const QMargins &margins,
            Project &project,
            QWidget *parent = 0);
    virtual ~SourceWidgetView();
    void setViewportOrigin(QPoint pt);
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
            const QRegion &paintRegion,
            RegexMatchList::iterator &findMatch);

    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void navMouseSingleDownEvent(QMouseEvent *event, QPoint virtualPos);
    void navMouseDoubleDownEvent(QMouseEvent *event, QPoint virtualPos);
    void navMouseTripleDownEvent(QMouseEvent *event, QPoint virtualPos);
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
    SourceWidgetTextPalette m_textPalette;
    QPoint m_viewportOrigin;
    QMargins m_margins;
    Project &m_project;
    File *m_file;
    std::vector<SourceWidgetTextPalette::Color> m_syntaxColoring;
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

class SourceWidget : public QAbstractScrollArea
{
    Q_OBJECT
public:
    explicit SourceWidget(Project &project, QWidget *parent = 0);
    void setFile(File *file);
    File *file() { return sourceWidgetView().file(); }
    void selectIdentifier(
            int line,
            int column,
            int endColumn,
            bool forceCenter);
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
    void scrollContentsBy(int dx, int dy);

private:
    QSize estimatedViewportSize();
    void updateScrollBars();
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
    void ensureVisible(QPoint pt, int xMargin = 50, int yMargin = 50);
    void ensureSelectedMatchVisible();

private:
    SourceWidgetLineArea *m_lineArea;
    SourceWidgetView *m_view;
    Project &m_project;

    // The view origin and top-left file offset when the user began searching.
    // Clearing the search returns the view to this origin.  Whenever the
    // search regex changes, the SourceWidget uses this file offset to decide
    // which match to select.
    QPoint m_findStartOrigin;
    int m_findStartOffset;

#if NAV_MACSCROLLOPTIMIZATION_HACK
    QWidget *m_macScrollOptimizationHack;
#endif

    friend class SourceWidgetLineArea;
};

} // namespace Nav

#endif // NAV_SOURCEWIDGET_H
