#include "SourceWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QPainter>
#include <QLabel>
#include <QTextBlock>
#include <QScrollBar>
#include <cassert>
#include <memory>
#include <unordered_map>
#include <re2/prog.h>
#include <re2/re2.h>
#include <re2/regexp.h>

#include "CXXSyntaxHighlighter.h"
#include "File.h"
#include "Misc.h"
#include "Project.h"
#include "Project-inl.h"
#include "Ref.h"
#include "Regex.h"
#include "RegexMatchList.h"
#include "ReportRefList.h"
#include "TableReportWindow.h"
#include "MainWindow.h"

namespace Nav {


///////////////////////////////////////////////////////////////////////////////
// Miscellaneous

const int kTabStopSize = 8;

static inline QSize marginsToSize(const QMargins &margins)
{
    return QSize(margins.left() + margins.right(),
                 margins.top() + margins.bottom());
}

// Measure the size of the line after expanding tab stops.
static int measureLineLength(StringRef line, int tabStopSize)
{
    int pos = 0;
    for (size_t i = 0; i < line.size(); ++i) {
        if (line[i] == '\t')
            pos = (pos + tabStopSize) / tabStopSize * tabStopSize;
        else
            pos++;
    }
    return pos;
}

static Qt::GlobalColor colorForSyntaxKind(CXXSyntaxHighlighter::Kind kind)
{
    switch (kind) {
    case CXXSyntaxHighlighter::KindComment:
    case CXXSyntaxHighlighter::KindQuoted:
        return Qt::darkGreen;
    case CXXSyntaxHighlighter::KindNumber:
    case CXXSyntaxHighlighter::KindDirective:
        return Qt::darkBlue;
    case CXXSyntaxHighlighter::KindKeyword:
        return Qt::darkYellow;
    default:
        // Qt::black is later converted to the foregroundRole() color.
        return Qt::black;
    }
}

namespace {

class ColorForRef {
public:
    ColorForRef(Project &project) : m_project(project) {
        addMapping("GlobalVariable", Qt::darkCyan);
        addMapping("Field", Qt::darkRed);
        addMapping("Namespace", Qt::darkMagenta);
        addMapping("Struct", Qt::darkMagenta);
        addMapping("Class", Qt::darkMagenta);
        addMapping("Union", Qt::darkMagenta);
        addMapping("Enum", Qt::darkMagenta);
        addMapping("Typedef", Qt::darkMagenta);
        addMapping("Macro", Qt::darkBlue);
    }

    Qt::GlobalColor color(const Ref &ref) const {
        auto it = m_map.find(m_project.querySymbolType(ref.symbolID()));
        return it == m_map.end() ? Qt::transparent : it->second;
    }

private:
    void addMapping(const char *symbolType, Qt::GlobalColor color) {
        indexdb::ID symbolTypeID = m_project.getSymbolTypeID(symbolType);
        if (symbolTypeID != indexdb::kInvalidID)
            m_map[symbolTypeID] = color;
    }

    Project &m_project;
    std::unordered_map<indexdb::ID, Qt::GlobalColor> m_map;
};

static inline int utf8CharLen(const char *pch)
{
    unsigned char ch = *pch;
    if ((ch & 0x80) == 0)
        return 1;
    else if ((ch & 0xE0) == 0xC0)
        return 2;
    else if ((ch & 0xF0) == 0xE0)
        return 3;
    else if ((ch & 0xF8) == 0xF0)
        return 4;
    else if ((ch & 0xFC) == 0xF8)
        return 5;
    else if ((ch & 0xFE) == 0xFC)
        return 6;
    // Invalid UTF-8.
    return 1;
}

static inline int utf8PrevCharLen(const char *nextChar, int maxLen)
{
    assert(maxLen > 0);
    maxLen = std::min(maxLen, 6);
    int len = 1;
    while (len < maxLen && (nextChar[-len] & 0xC0) == 0x80)
        len++;
    return len;
}

static inline bool isUtf8WordChar(const char *pch)
{
    char ch = *pch;
    return ch == '_' ||
            (ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9');
}


///////////////////////////////////////////////////////////////////////////////
// <anon>::LineLayout

class LineLayout {
public:
    // line is 0-based.
    LineLayout(
            const QFont &font,
            const QMargins &margins,
            File &file,
            int line) :
        m_twc(TextWidthCalculator::getCachedTextWidthCalculator(font)),
        m_fm(font)
    {
        m_lineContent = file.lineContent(line);
        m_lineHeight = effectiveLineSpacing(m_fm);
        m_lineTop = margins.top() + line * m_lineHeight;
        m_lineBaselineY = m_lineTop + m_fm.ascent();
        m_lineLeftMargin = margins.left();
        m_lineStartIndex = file.lineStart(line);
        m_tabStopPx = m_fm.width(QChar(' ')) * kTabStopSize;
        m_charIndex = -1;
        m_charLeft = 0;
        m_charWidth = 0;
    }

    bool hasMoreChars()
    {
        return nextCharIndex() < static_cast<int>(m_lineContent.size());
    }

    void advanceChar()
    {
        // Skip over the previous character.
        m_charIndex = nextCharIndex();
        m_charLeft += m_charWidth;
        // Analyze the next character.
        const char *const pch = &m_lineContent[m_charIndex];
        if (*pch == '\t') {
            m_charText.clear();
            m_charWidth =
                    (m_charLeft + m_tabStopPx) /
                    m_tabStopPx * m_tabStopPx - m_charLeft;
        } else {
            int len = charByteLen();
            m_charText.resize(len);
            for (int i = 0; i < len; ++i)
                m_charText[i] = pch[i];
            m_charWidth = m_twc.calculate(m_charText.c_str());
        }
    }

    int charColumn()                { return m_charIndex; }
    int charFileIndex()             { return m_lineStartIndex + m_charIndex; }
    int charLeft()                  { return m_lineLeftMargin + m_charLeft; }
    int charWidth()                 { return m_charWidth; }
    int lineTop()                   { return m_lineTop; }
    int lineHeight()                { return m_lineHeight; }
    int lineBaselineY()             { return m_lineBaselineY; }
    const std::string &charText()   { return m_charText; }

private:
    int charByteLen()
    {
        return std::min<int>(utf8CharLen(&m_lineContent[m_charIndex]),
                             m_lineContent.size() - m_charIndex);
    }

    int nextCharIndex()             { return m_charIndex + charByteLen(); }

private:
    TextWidthCalculator m_twc;
    QFontMetrics m_fm;
    StringRef m_lineContent;
    int m_lineTop;
    int m_lineHeight;
    int m_lineBaselineY;
    int m_lineLeftMargin;
    int m_lineStartIndex;
    int m_tabStopPx;
    int m_charIndex;
    int m_charLeft;
    int m_charWidth;
    std::string m_charText;
};

} // anonymous namespace


///////////////////////////////////////////////////////////////////////////////
// SourceWidgetLineArea

QSize SourceWidgetLineArea::sizeHint() const
{
    QFontMetrics fm = fontMetrics();
    return QSize(fm.width('9') * QString::number(m_lineCount).size(),
                 fm.height() * std::max(1, m_lineCount)) +
            marginsToSize(m_margins);
}

void SourceWidgetLineArea::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    QFontMetrics fm = fontMetrics();
    const int ch = effectiveLineSpacing(fm);
    const int cw = fm.width('9');
    const int ca = fm.ascent();
    const int line1 = std::max(0, event->rect().top() / ch - 2);
    const int line2 = std::min(m_lineCount - 1, event->rect().bottom() / ch + 2);
    const int thisWidth = width();

    p.setPen(QColor(Qt::darkGray));
    for (int line = line1; line <= line2; ++line) {
        QString text = QString::number(line + 1);
        p.drawText(thisWidth - (text.size() * cw) - m_margins.right(),
                   ch * line + ca + m_margins.top(),
                   text);
    }
}


///////////////////////////////////////////////////////////////////////////////
// SourceWidgetView

SourceWidgetView::SourceWidgetView(const QMargins &margins, Project &project) :
    m_margins(margins),
    m_project(project),
    m_file(NULL),
    m_mouseHoveringInWidget(false),
    m_selectingMode(SM_Inactive),
    m_selectedMatchIndex(-1)
{
    setBackgroundRole(QPalette::NoRole);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);
    updateFindMatches();
}

// Declare out-of-line destructor where member std::unique_ptr's types are
// defined.
SourceWidgetView::~SourceWidgetView()
{
}

void SourceWidgetView::setFile(File *file)
{
    if (m_file == file)
        return;

    m_file = file;
    m_maxLineLength = 0;
    m_selectingMode = SM_Inactive;
    m_selectedRange = FileRange();
    updateSelectionAndHover();

    if (m_file != NULL) {
        const std::string &content = m_file->content();
        auto syntaxColoringKind = CXXSyntaxHighlighter::highlight(content);

        // Color characters according to the lexed character kind.
        m_syntaxColoring.resize(content.size());
        for (size_t i = 0; i < syntaxColoringKind.size(); ++i) {
            m_syntaxColoring[i] = colorForSyntaxKind(syntaxColoringKind[i]);
        }

        // Color characters according to the index's refs.
        ColorForRef colorForRef(m_project);
        m_project.queryFileRefs(*m_file, [=](const Ref &ref) {
            if (ref.line() > m_file->lineCount())
                return;
            int offset = m_file->lineStart(ref.line() - 1);
            Qt::GlobalColor color = colorForRef.color(ref);
            if (color != Qt::transparent) {
                for (int i = ref.column() - 1,
                        iEnd = std::min(ref.endColumn() - 1,
                                        m_file->lineLength(ref.line() - 1));
                        i < iEnd; ++i) {
                    m_syntaxColoring[offset + i] = color;
                }
            }
        });

        // Measure the longest line.
        int maxLength = 0;
        for (int i = 0, lineCount = m_file->lineCount(); i < lineCount; ++i) {
            maxLength = std::max(
                        maxLength,
                        measureLineLength(m_file->lineContent(i),
                                          kTabStopSize));
        }
        m_maxLineLength = maxLength;
    }

    updateFindMatches();
    updateGeometry();
    update();
}

void SourceWidgetView::paintEvent(QPaintEvent *event)
{
    if (m_file == NULL)
        return;

    const int lineSpacing = effectiveLineSpacing(fontMetrics());
    QPainter painter(this);

    // Paint lines in the clip region.
    const int line1 = std::max(event->rect().y() / lineSpacing - 2, 0);
    const int line2 = std::min(event->rect().bottom() / lineSpacing + 2,
                               m_file->lineCount() - 1);
    if (line1 > line2)
        return;

    // Find the first interesting find match using binary search.  Scanning
    // the matches linearly is especially slow because it would force
    // evaluation of regex start positions.
    const int line1Offset = FileLocation(line1, 0).toOffset(*m_file);
    RegexMatchList::iterator findMatch =
            std::lower_bound(
                m_findMatches.begin(),
                m_findMatches.end(),
                std::make_pair(line1Offset, line1Offset));
    if (findMatch > m_findMatches.begin())
        --findMatch;

    for (int line = line1; line <= line2; ++line)
        paintLine(painter, line, event->rect(), findMatch);
}

int SourceWidgetView::lineTop(int line)
{
    return effectiveLineSpacing(fontMetrics()) * line + m_margins.top();
}

// line is 0-based.
void SourceWidgetView::paintLine(
        QPainter &painter,
        int line,
        const QRect &paintRect,
        RegexMatchList::iterator &findMatch)
{
    const int selStartOff = m_selectedRange.start.toOffset(*m_file);
    const int selEndOff = m_selectedRange.end.toOffset(*m_file);
    const int hovStartOff = m_hoverHighlightRange.start.toOffset(*m_file);
    const int hovEndOff = m_hoverHighlightRange.end.toOffset(*m_file);
    const QBrush matchBrush(Qt::yellow);
    const QBrush selectedMatchBrush(QColor(255, 140, 0));
    const QBrush hoverBrush(QColor(200, 200, 200));

    // Fill the line's background.
    {
        LineLayout lay(font(), m_margins, *m_file, line);
        while (lay.hasMoreChars()) {
            lay.advanceChar();
            if (lay.charLeft() > paintRect.right())
                break;
            if (lay.charLeft() + lay.charWidth() <= paintRect.left())
                continue;

            const int charFileIndex = lay.charFileIndex();
            const QBrush *fillBrush = NULL;

            // Find match background.
            for (; findMatch < m_findMatches.end(); ++findMatch) {
                if (findMatch->first > charFileIndex)
                    break;
                if (findMatch->second <= charFileIndex)
                    continue;
                // This find match covers the current character.
                if (findMatch - m_findMatches.begin() == m_selectedMatchIndex)
                    fillBrush = &selectedMatchBrush;
                else
                    fillBrush = &matchBrush;
                break;
            }

            // Hover and selection backgrounds.
            if (charFileIndex >= hovStartOff && charFileIndex < hovEndOff)
                fillBrush = &hoverBrush;
            if (charFileIndex >= selStartOff && charFileIndex < selEndOff)
                fillBrush = &palette().highlight();

            if (fillBrush != NULL) {
                painter.fillRect(lay.charLeft(),
                                 lay.lineTop(),
                                 lay.charWidth(),
                                 lay.lineHeight(),
                                 *fillBrush);
            }
        }
    }

    // Draw characters.
    {
        LineLayout lay(font(), m_margins, *m_file, line);
        QPalette pal(palette());
        QColor defaultTextColor(pal.color(foregroundRole()));
        QColor currentColor(defaultTextColor);
        QColor charColor;
        painter.setPen(currentColor);
        while (lay.hasMoreChars()) {
            lay.advanceChar();
            if (lay.charLeft() > paintRect.right())
                break;
            if (lay.charLeft() + lay.charWidth() <= paintRect.left())
                continue;
            if (!lay.charText().empty()) {
                FileLocation loc(line, lay.charColumn());
                Qt::GlobalColor gc = m_syntaxColoring[lay.charFileIndex()];
                if (gc == Qt::black)
                    charColor = defaultTextColor;
                else
                    charColor = gc;

                // Override the color for selected text.
                if (loc >= m_selectedRange.start && loc < m_selectedRange.end)
                    charColor = pal.highlightedText().color();

                // Set the painter pen when the color changes.
                if (charColor != currentColor) {
                    painter.setPen(charColor);
                    currentColor = charColor;
                }

                painter.drawText(
                            lay.charLeft(),
                            lay.lineBaselineY(),
                            lay.charText().c_str());
            }
        }
    }
}

FileLocation SourceWidgetView::hitTest(QPoint pixel, bool roundToNearest)
{
    if (m_file == NULL)
        return FileLocation();
    QFontMetrics fm = fontMetrics();
    int line = (pixel.y() - m_margins.top()) / effectiveLineSpacing(fm);
    if (line < 0) {
        return FileLocation(0, 0);
    } else if (line >= m_file->lineCount()) {
        return FileLocation(m_file->lineCount(), 0);
    } else {
        LineLayout lay(font(), m_margins, *m_file, line);
        while (lay.hasMoreChars()) {
            lay.advanceChar();
            int charWidth = lay.charWidth();
            if (roundToNearest)
                charWidth = (charWidth >> 1) + 1;
            if (pixel.x() < lay.charLeft() + charWidth)
                return FileLocation(line, lay.charColumn());
        }
        return FileLocation(line, m_file->lineLength(line));
    }
}

QPoint SourceWidgetView::locationToPoint(FileLocation loc)
{
    if (m_file == NULL || loc.isNull())
        return QPoint(m_margins.left(), m_margins.top());
    if (loc.line >= m_file->lineCount())
        return QPoint(m_margins.left(), lineTop(m_file->lineCount()));
    LineLayout lay(font(), m_margins, *m_file, loc.line);
    while (lay.hasMoreChars()) {
        lay.advanceChar();
        if (lay.charColumn() == loc.column)
            return QPoint(lay.charLeft(), lineTop(loc.line));
    }
    return QPoint(lay.charLeft() + lay.charWidth(), lineTop(loc.line));
}

QSize SourceWidgetView::sizeHint() const
{
    if (m_file == NULL)
        return marginsToSize(m_margins);
    QFontMetrics fontMetrics(font());
    return QSize(m_maxLineLength * fontMetrics.width(' '),
                 m_file->lineCount() * effectiveLineSpacing(fontMetrics)) +
            marginsToSize(m_margins);
}

void SourceWidgetView::setFindRegex(const Regex &findRegex)
{
    if (m_findRegex == findRegex)
        return;
    m_findRegex = findRegex;
    updateFindMatches();
    update();
}

void SourceWidgetView::setSelectedMatchIndex(int index)
{
    if (m_findMatches.empty() && (index != -1))
        return;
    if (index == m_selectedMatchIndex)
        return;

    updateRange(matchFileRange(m_selectedMatchIndex));
    m_selectedMatchIndex = index;
    updateRange(matchFileRange(m_selectedMatchIndex));

    emit findMatchSelectionChanged(m_selectedMatchIndex);
}

void SourceWidgetView::copy()
{
    StringRef text = rangeText(m_selectedRange);
    if (text.size() > 0) {
        QString qtext = QString::fromAscii(text.data(), text.size());
        QApplication::clipboard()->setText(qtext);
    }
}

void SourceWidgetView::setSelection(const FileRange &fileRange)
{
    if (m_selectedRange == fileRange)
        return;
    updateRange(m_selectedRange);
    updateRange(fileRange);
    m_selectedRange = fileRange;
}

void SourceWidgetView::setHoverHighlight(const FileRange &fileRange)
{
    if (m_hoverHighlightRange == fileRange)
        return;
    updateRange(m_hoverHighlightRange);
    updateRange(fileRange);
    m_hoverHighlightRange = fileRange;
}

void SourceWidgetView::updateRange(const FileRange &range)
{
    if (m_file == NULL || range.isEmpty())
        return;
    const int lineHeight = effectiveLineSpacing(fontMetrics());
    assert(!range.start.isNull() && !range.end.isNull());
    if (range.start.line == range.end.line) {
        QPoint pt1 = locationToPoint(range.start);
        QPoint pt2 = locationToPoint(range.end);
        update(pt1.x(), pt1.y(), pt2.x() - pt1.x(), lineHeight);
    } else {
        int y1 = lineTop(range.start.line);
        int y2 = lineTop(range.end.line) + lineHeight;
        update(0, y1, width(), y2 - y1);
    }
}

StringRef SourceWidgetView::rangeText(const FileRange &range)
{
    if (m_file == NULL || range.isEmpty())
        return StringRef();
    assert(!range.start.isNull() && !range.end.isNull());
    size_t offset1 = range.start.toOffset(*m_file);
    size_t offset2 = range.end.toOffset(*m_file);
    const std::string &content = m_file->content();
    offset1 = std::min(offset1, content.size());
    offset2 = std::min(offset2, content.size());
    return StringRef(content.c_str() + offset1, offset2 - offset1);
}

FileRange SourceWidgetView::findRefAtLocation(const FileLocation &pt)
{
    if (m_file == NULL || !pt.doesPointAtChar(*m_file))
        return FileRange();
    Ref bestRef;
    const int fileLine = pt.line + 1;
    const int fileColumn = pt.column + 1;
    m_project.queryFileRefs(
                *m_file,
                [fileLine, fileColumn, &bestRef](const Ref &ref) {
        if (fileColumn >= ref.column() && fileColumn < ref.endColumn()) {
            if (bestRef.isNull() ||
                    ref.column() > bestRef.column() ||
                    (ref.column() == bestRef.column() &&
                            ref.endColumn() < bestRef.endColumn())) {
                bestRef = ref;
            }
        }
    }, fileLine, fileLine);
    if (bestRef.isNull())
        return FileRange();
    FileLocation loc1(bestRef.line() - 1, bestRef.column() - 1);
    FileLocation loc2(bestRef.line() - 1, bestRef.endColumn() - 1);
    return FileRange(loc1, loc2);
}

FileRange SourceWidgetView::findRefAtPoint(QPoint pt)
{
    if (m_file != NULL) {
        FileLocation loc = hitTest(pt);
        if (loc.doesPointAtChar(*m_file))
            return findRefAtLocation(loc);
    }
    return FileRange();
}

std::set<std::string> SourceWidgetView::findSymbolsAtRange(
        const FileRange &range)
{
    std::set<std::string> result;
    if (!range.isEmpty() && range.start.line == range.end.line) {
        m_project.queryFileRefs(
                    *m_file,
                    [&result, &range](const Ref &ref) {
            if (ref.column() == range.start.column + 1 &&
                    ref.endColumn() == range.end.column + 1)
                result.insert(ref.symbolCStr());
        }, range.start.line + 1, range.start.line + 1);
    }
    return result;
}

FileRange SourceWidgetView::findWordAtLocation(FileLocation loc)
{
    assert(m_file != NULL);
    if (!loc.doesPointAtChar(*m_file))
        return FileRange(loc, loc);
    StringRef lineContent = m_file->lineContent(loc.line);
    const int lineLength = lineContent.size();
    if (!isUtf8WordChar(&lineContent[loc.column])) {
        int charLen = utf8CharLen(&lineContent[loc.column]);
        return FileRange(loc, FileLocation(loc.line,
            loc.column + std::min(charLen, lineLength - loc.column)));
    }
    int col1 = loc.column;
    int col2 = loc.column;
    while (col1 > 0) {
        int prevCol1 = col1 - utf8PrevCharLen(&lineContent[col1], col1);
        if (!isUtf8WordChar(&lineContent[prevCol1]))
            break;
        col1 = prevCol1;
    }
    while (col2 < lineLength) {
        int charLen = utf8CharLen(&lineContent[col2]);
        if (charLen > lineLength - col2)
            break;
        if (!isUtf8WordChar(&lineContent[col2]))
            break;
        col2 += charLen;
    }
    return FileRange(FileLocation(loc.line, col1),
                     FileLocation(loc.line, col2));
}

void SourceWidgetView::mousePressEvent(QMouseEvent *event)
{
    if (m_tripleClickTime.elapsed() < QApplication::doubleClickInterval() &&
            (event->pos() - m_tripleClickPoint).manhattanLength() <
                QApplication::startDragDistance()) {
        m_tripleClickTime = QTime();
        m_tripleClickPoint = QPoint();
        navMouseTripleDownEvent(event);
    } else {
        navMouseSingleDownEvent(event);
    }
}

void SourceWidgetView::mouseDoubleClickEvent(QMouseEvent *event)
{
    m_tripleClickTime.start();
    m_tripleClickPoint = event->pos();
    navMouseDoubleDownEvent(event);
}

void SourceWidgetView::navMouseSingleDownEvent(QMouseEvent *event)
{
    FileLocation loc = hitTest(event->pos());
    FileRange refRange = findRefAtLocation(loc);
    if (!refRange.isEmpty()) {
        m_selectingMode = SM_Ref;
        setSelection(refRange);
    } else {
        if (event->button() == Qt::LeftButton) {
            m_selectingMode = SM_Char;
            m_selectingAnchor = event->pos();
        } else if (event->button() == Qt::RightButton &&
                !m_selectedRange.isEmpty() &&
                (loc < m_selectedRange.start || loc >= m_selectedRange.end)) {
            setSelection(FileRange());
        }
    }
    updateSelectionAndHover(event->pos());
}

void SourceWidgetView::navMouseDoubleDownEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_selectingMode = SM_Word;
        m_selectingAnchor = event->pos();
        updateSelectionAndHover(event->pos());
    }
}

void SourceWidgetView::navMouseTripleDownEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_selectingMode = SM_Line;
        m_selectingAnchor = event->pos();
        updateSelectionAndHover(event->pos());
    }
}

void SourceWidgetView::mouseMoveEvent(QMouseEvent *event)
{
    updateSelectionAndHover(event->pos());
    if (m_selectingMode != SM_Inactive && m_selectingMode != SM_Ref)
        emit pointSelected(event->pos());
}

void SourceWidgetView::moveEvent(QMoveEvent *event)
{
    updateSelectionAndHover();
}

bool SourceWidgetView::event(QEvent *event)
{
    if (event->type() == QEvent::HoverMove ||
            event->type() == QEvent::HoverEnter) {
        m_mouseHoveringInWidget = true;
        updateSelectionAndHover();
    }
    if (event->type() == QEvent::HoverLeave) {
        m_mouseHoveringInWidget = false;
        updateSelectionAndHover();
    }
    return QWidget::event(event);
}

void SourceWidgetView::updateSelectionAndHover()
{
    updateSelectionAndHover(mapFromGlobal(QCursor::pos()));
}

// Update the selected range, hover highlight range, and mouse cursor to
// account for a mouse move to the given position.
void SourceWidgetView::updateSelectionAndHover(QPoint mousePos)
{
    if (m_file == NULL)
        return;
    if (m_selectingMode == SM_Inactive) {
        FileRange word;
        if (rect().contains(mousePos) && m_mouseHoveringInWidget)
            word = findRefAtPoint(mousePos);
        setHoverHighlight(word);
        setCursor(word.isEmpty() ? Qt::ArrowCursor : Qt::PointingHandCursor);
        m_selectingAnchor = QPoint();
    } else if (m_selectingMode == SM_Ref) {
        setHoverHighlight(FileRange());
        if (!m_selectedRange.isEmpty()) {
            FileRange word = findRefAtPoint(mousePos);
            if (word == m_selectedRange) {
                setCursor(Qt::PointingHandCursor);
            } else {
                setSelection(FileRange());
                setCursor(Qt::ArrowCursor);
            }
        } else {
            setCursor(Qt::ArrowCursor);
        }
        m_selectingAnchor = QPoint();
    } else {
        setHoverHighlight(FileRange());
        if (m_selectingMode == SM_Char) {
            setCursor(Qt::ArrowCursor);
            FileLocation loc1 = hitTest(m_selectingAnchor,
                                        /*roundToNearest=*/true);
            FileLocation loc2 = hitTest(mousePos,
                                        /*roundToNearest=*/true);
            setSelection(FileRange(std::min(loc1, loc2),
                                   std::max(loc1, loc2)));
        } else if (m_selectingMode == SM_Word) {
            setCursor(Qt::ArrowCursor);
            FileRange loc1 = findWordAtLocation(hitTest(m_selectingAnchor));
            FileRange loc2 = findWordAtLocation(hitTest(mousePos));
            setSelection(FileRange(std::min(loc1.start, loc2.start),
                                   std::max(loc1.end, loc2.end)));
        } else if (m_selectingMode == SM_Line) {
            setCursor(Qt::ArrowCursor);
            FileLocation loc1 = hitTest(m_selectingAnchor);
            FileLocation loc2 = hitTest(mousePos);
            int line1 = std::min(loc1.line, loc2.line);
            int line2 = std::max(loc1.line, loc2.line);
            FileLocation lineLoc1 = FileLocation(line1, 0);
            FileLocation lineLoc2;
            if (line2 < m_file->lineCount())
                lineLoc2 = FileLocation(line2, m_file->lineLength(line2));
            else
                lineLoc2 = FileLocation(line2, 0);
            setSelection(FileRange(lineLoc1, lineLoc2));
        } else {
            assert(false && "Invalid m_selectingMode");
        }
        // Update the selection clipboard.
        StringRef text = rangeText(m_selectedRange);
        if (text.size() > 0) {
            QString qtext = QString::fromAscii(text.data(), text.size());
            QApplication::clipboard()->setText(qtext, QClipboard::Selection);
        }
    }
}

void SourceWidgetView::mouseReleaseEvent(QMouseEvent *event)
{
    SelectingMode oldMode = m_selectingMode;
    m_selectingMode = SM_Inactive;
    updateSelectionAndHover(event->pos());

    if (oldMode == SM_Ref && !m_selectedRange.isEmpty()) {
        FileRange identifierClicked;
        if (m_selectedRange == findRefAtPoint(event->pos()))
            identifierClicked = m_selectedRange;
        setSelection(FileRange());

        // Delay the event handling as long as possible.  Clicking a symbol is
        // likely to cause a jump to another location, which will change the
        // selection (and perhaps the file being displayed).
        if (!identifierClicked.isEmpty()) {
            std::set<std::string> symbols =
                    findSymbolsAtRange(identifierClicked);
            // TODO: Is this behavior really ideal in the case that one
            // location maps to multiple symbols?
            if (symbols.size() == 1) {
                Ref ref = m_project.findSingleDefinitionOfSymbol(
                            symbols.begin()->c_str());
                theMainWindow->navigateToRef(ref);
            }
        }
    }
}

void SourceWidgetView::contextMenuEvent(QContextMenuEvent *event)
{
    m_selectingMode = SM_Inactive;
    m_tripleClickTime = QTime();
    m_tripleClickPoint = QPoint();
    m_hoverHighlightRange = FileRange();
    setCursor(Qt::ArrowCursor);

    if (m_selectedRange.isEmpty()) {
        QMenu *menu = new QMenu();
        bool backEnabled = false;
        bool forwardEnabled = false;
        emit areBackAndForwardEnabled(backEnabled, forwardEnabled);
        menu->addAction(QIcon::fromTheme("go-previous"), "&Back",
                        this, SIGNAL(goBack()))->setEnabled(backEnabled);
        menu->addAction(QIcon::fromTheme("go-next"), "&Forward",
                        this, SIGNAL(goForward()))->setEnabled(forwardEnabled);
        menu->addAction("&Copy File Path", this, SIGNAL(copyFilePath()));
        menu->addAction("Reveal in &Sidebar", this, SIGNAL(revealInSideBar()));
        menu->exec(event->globalPos());
        delete menu;
    } else {
        QMenu *menu = new QMenu();
        std::set<std::string> symbols = findSymbolsAtRange(m_selectedRange);
        if (symbols.empty()) {
            menu->addAction(QIcon::fromTheme("edit-copy"), "&Copy",
                            this, SLOT(copy()));
        } else {
            for (const auto &symbol : symbols) {
                QString actionText = QString::fromStdString(symbol);
                actionText.replace('&', "&&");
                QAction *action = menu->addAction(actionText);
                action->setEnabled(false);
                QFont f = action->font();
                f.setBold(true);
                action->setFont(f);
                menu->addSeparator();
                action = menu->addAction("Cross-references...",
                                         this,
                                         SLOT(actionCrossReferences()));
                action->setData(symbol.c_str());
                menu->addSeparator();
            }
        }
        menu->exec(event->globalPos());
        delete menu;
    }

    update();
}

void SourceWidgetView::updateFindMatches()
{
    // Search the file with the new regex.
    if (m_file == NULL) {
        m_findMatches = RegexMatchList();
    } else {
        m_findMatches = RegexMatchList(m_file->content(), m_findRegex);
    }

    // Tentatively clear the match selection.  If updateFindMatches() was
    // called because the user changed the search filter, then the parent
    // SourceWidget class will take care of picking the most appropriate
    // match to select, as well as scrolling it into view.
    m_selectedMatchIndex = -1;

    emit findMatchListChanged();
}

FileRange SourceWidgetView::matchFileRange(int index)
{
    if (index == -1) {
        return FileRange();
    } else {
        assert(m_file != NULL);
        FileLocation loc1(*m_file, m_findMatches[index].first);
        FileLocation loc2(*m_file, m_findMatches[index].second);
        return FileRange(loc1, loc2);
    }
}

void SourceWidgetView::actionCrossReferences()
{
    QAction *action = qobject_cast<QAction*>(sender());
    QString symbol = action->data().toString();
    TableReportWindow *tw = new TableReportWindow;
    ReportRefList *r = new ReportRefList(*theProject, symbol, tw);
    tw->setTableReport(r);
    tw->show();
}


///////////////////////////////////////////////////////////////////////////////
// SourceWidget

SourceWidget::SourceWidget(Project &project, QWidget *parent) :
    QScrollArea(parent),
    m_project(project),
    m_findStartOffset(0)
{
    setWidgetResizable(false);
    setWidget(new SourceWidgetView(QMargins(4, 4, 4, 4), project));
    setBackgroundRole(QPalette::Base);
    setViewportMargins(30, 0, 0, 0);
    m_lineAreaViewport = new QWidget(this);
    m_lineArea = new SourceWidgetLineArea(QMargins(4, 4, 4, 4), m_lineAreaViewport);
    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(layoutSourceWidget()));

    // Configure the widgets to use a small monospace font.
    QFont font;
    font.setFamily("Monospace");
    font.setPointSize(8);
    font.setKerning(false);
    widget()->setFont(font);
    m_lineArea->setFont(font);

    connect(&sourceWidgetView(), SIGNAL(goBack()), SIGNAL(goBack()));
    connect(&sourceWidgetView(), SIGNAL(goForward()), SIGNAL(goForward()));
    connect(&sourceWidgetView(),
            SIGNAL(areBackAndForwardEnabled(bool&,bool&)),
            SIGNAL(areBackAndForwardEnabled(bool&,bool&)));
    connect(&sourceWidgetView(), SIGNAL(copyFilePath()), SIGNAL(copyFilePath()));
    connect(&sourceWidgetView(), SIGNAL(revealInSideBar()), SIGNAL(revealInSideBar()));
    connect(&sourceWidgetView(),
            SIGNAL(pointSelected(QPoint)),
            SLOT(viewPointSelected(QPoint)));
    connect(&sourceWidgetView(),
            SIGNAL(findMatchSelectionChanged(int)),
            SIGNAL(findMatchSelectionChanged(int)));
    connect(&sourceWidgetView(),
            SIGNAL(findMatchListChanged()),
            SIGNAL(findMatchListChanged()));
    layoutSourceWidget();
}

void SourceWidget::setFile(File *file)
{
    if (this->file() != file) {
        verticalScrollBar()->setValue(0);
        sourceWidgetView().setFile(file);
        m_lineArea->setLineCount(file != NULL ? file->lineCount() : 0);
        layoutSourceWidget();
        m_findStartOffset = -1;
        m_findStartOrigin = QPoint();

        emit fileChanged(file);
    }
}

SourceWidgetView &SourceWidget::sourceWidgetView()
{
    return *qobject_cast<SourceWidgetView*>(widget());
}

void SourceWidget::layoutSourceWidget(void)
{
    QSize lineAreaSizeHint = m_lineArea->sizeHint();
    m_lineAreaViewport->setGeometry(
                0,
                viewport()->pos().y(),
                lineAreaSizeHint.width(),
                viewport()->height());
    m_lineArea->setGeometry(
                0,
                -verticalScrollBar()->value(),
                lineAreaSizeHint.width(),
                lineAreaSizeHint.height());
    setViewportMargins(lineAreaSizeHint.width(), 0, 0, 0);

    // Hack around a Qt problem.  QScrollArea can be configured to resize
    // the widget automatically (i.e. setWidgetResizable(true)), but this
    // isn't working because the window isn't resized soon enough.  (Maybe
    // Qt is waiting until it returns to the event loop?)  Sometimes we
    // want to scroll to the end of the file, but we can't because that's
    // beyond the end of the the stale widget size Qt is keeping.
    //
    // Basically, Qt has a cached widget size that it computed from the
    // sizeHint.  My code notifies Qt that the sizeHint has changed, then
    // it asks Qt to do something involving the widget size, and Qt *still*
    // uses the cached, invalid size.

    QSize sizeHint = sourceWidgetView().sizeHint();
    sizeHint = sizeHint.expandedTo(viewport()->size());
    sourceWidgetView().resize(sizeHint);
}

void SourceWidget::viewPointSelected(QPoint point)
{
    ensureVisible(point.x(), point.y(), 0, 0);
}

void SourceWidget::resizeEvent(QResizeEvent *event)
{
    layoutSourceWidget();
    QScrollArea::resizeEvent(event);
}

void SourceWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Home)
        verticalScrollBar()->setValue(0);
    else if (event->key() == Qt::Key_End)
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    else
        QScrollArea::keyPressEvent(event);
}

// Line and column indices are 1-based.
void SourceWidget::selectIdentifier(
        int line,
        int column,
        int endColumn,
        bool forceCenter)
{
    SourceWidgetView &w = sourceWidgetView();

    FileRange r(FileLocation(line - 1, column - 1),
                FileLocation(line - 1, endColumn - 1));
    w.setSelection(r);

    // Scroll the selected identifier into range.  If it's already visible,
    // do nothing.  Otherwise, center the identifier vertically in the
    // viewport, and scroll as far left as possible.
    QPoint wordTopLeft = w.locationToPoint(r.start);
    QPoint wordBottomRight = w.locationToPoint(r.end);
    wordBottomRight.ry() += effectiveLineSpacing(fontMetrics());
    QRect viewportRect(viewportOrigin(), viewport()->size());
    if (forceCenter ||
            !viewportRect.contains(wordTopLeft) ||
            !viewportRect.contains(wordBottomRight)) {
        int wordCenterY = ((wordTopLeft + wordBottomRight) / 2).y();
        int originX = std::min(wordBottomRight.x() + 20 - viewport()->width(),
                               wordTopLeft.x() - 20);
        int originY = wordCenterY - viewport()->height() / 2;
        setViewportOrigin(QPoint(originX, originY));
    }
}

QPoint SourceWidget::viewportOrigin()
{
    return QPoint(horizontalScrollBar()->value(),
                  verticalScrollBar()->value());
}

void SourceWidget::setViewportOrigin(const QPoint &pt)
{
    horizontalScrollBar()->setValue(pt.x());
    verticalScrollBar()->setValue(pt.y());
}

// When the user starts a new search, record the viewport location so we can
// return to it if the user clears their search / backspaces through part of
// it.
void SourceWidget::recordFindStart()
{
    QPoint topLeft = viewportOrigin();
    topLeft.setY(topLeft.y() + effectiveLineSpacing(font()) - 1);
    m_findStartOffset =
            sourceWidgetView().hitTest(topLeft).toOffset(*file());
    m_findStartOrigin = viewportOrigin();
}

// Clear all find-related state.
void SourceWidget::endFind()
{
    m_findStartOffset = -1;
    m_findStartOrigin = QPoint();
    sourceWidgetView().setFindRegex(Regex());
}

void SourceWidget::setFindRegex(const Regex &findRegex, bool advanceToMatch)
{
    if (findRegex.empty() && m_findStartOffset != -1 && advanceToMatch)
        setViewportOrigin(m_findStartOrigin);
    const int previousIndex = sourceWidgetView().selectedMatchIndex();
    const int previousOffset = previousIndex == -1
            ? -1 : sourceWidgetView().findMatches()[previousIndex].first;
    sourceWidgetView().setFindRegex(findRegex);
    if (!advanceToMatch)
        return;
    if (m_findStartOffset == -1)
        recordFindStart();
    setSelectedMatchIndex(bestMatchIndex(previousOffset));
}

int SourceWidget::matchCount()
{
    return sourceWidgetView().findMatches().size();
}

int SourceWidget::selectedMatchIndex()
{
    return sourceWidgetView().selectedMatchIndex();
}

void SourceWidget::selectNextMatch()
{
    if (matchCount() == 0)
        return;
    if (m_findStartOffset == -1)
        recordFindStart();
    if (selectedMatchIndex() == -1)
        setSelectedMatchIndex(bestMatchIndex(/*previousMatchOffset=*/-1));
    else
        setSelectedMatchIndex((selectedMatchIndex() + 1) % matchCount());
}

void SourceWidget::selectPreviousMatch()
{
    if (matchCount() == 0)
        return;
    if (m_findStartOffset == -1)
        recordFindStart();
    if (selectedMatchIndex() == -1)
        setSelectedMatchIndex(bestMatchIndex(/*previousMatchOffset=*/-1));
    setSelectedMatchIndex(
                (selectedMatchIndex() + matchCount() - 1) % matchCount());
}

void SourceWidget::setSelectedMatchIndex(int index)
{
    sourceWidgetView().setSelectedMatchIndex(index);
    ensureSelectedMatchVisible();
}

int SourceWidget::bestMatchIndex(int previousMatchOffset)
{
    const auto &matches = sourceWidgetView().findMatches();
    if (matches.size() == 0)
        return -1;

    if (previousMatchOffset != -1) {
        // Look for the first match before the previous offset (wrapping around
        // if necessary).  If the first match is within the [FSO,PO] range,
        // select it.

        // Find the first match <= the previous offset.
        {
            auto it = std::lower_bound(
                        matches.begin(), matches.end(),
                        std::make_pair(previousMatchOffset + 1, 0));
            int index;
            if (it > matches.begin()) {
                --it;
                index = it - matches.begin();
            } else {
                index = matches.size() - 1;
            }
            const int offset = matches[index].first;
            if (m_findStartOffset <= previousMatchOffset) {
                if (offset >= m_findStartOffset &&
                        offset <= previousMatchOffset) {
                    return index;
                }
            } else {
                if (offset >= m_findStartOffset ||
                        offset <= previousMatchOffset) {
                    return index;
                }
            }
        }

        // Then look for the first match after the previous offset.
        {
            auto it = std::lower_bound(matches.begin(), matches.end(),
                                       std::make_pair(previousMatchOffset, 0));
            if (it == matches.end())
                it = matches.begin();
            return it - matches.begin();
        }
    }

    // If there was no previously selected match, look for the first match
    // after the starting offset.
    if (m_findStartOffset != -1) {
        auto it = std::lower_bound(matches.begin(), matches.end(),
                                   std::make_pair(m_findStartOffset, 0));
        if (it != matches.end())
            return it - matches.begin();
    }

    // If there was no match after the starting offset, use the first match.
    return 0;
}

void SourceWidget::ensureSelectedMatchVisible()
{
    int index = sourceWidgetView().selectedMatchIndex();
    if (index == -1)
        return;
    assert(file() != NULL);
    const auto &matches = sourceWidgetView().findMatches();

    FileLocation matchStartLoc(*file(), matches[index].first);
    FileLocation matchEndLoc(*file(), matches[index].second);
    QPoint matchStartPt = sourceWidgetView().locationToPoint(matchStartLoc);
    QPoint matchEndPt = sourceWidgetView().locationToPoint(matchEndLoc);
    matchEndPt.setY(matchEndPt.y() + effectiveLineSpacing(font()));

    QRect visibleViewRect(
                QPoint(horizontalScrollBar()->value(),
                       verticalScrollBar()->value()),
                viewport()->size());
    bool needScroll = !visibleViewRect.contains(matchStartPt) ||
                      !visibleViewRect.contains(matchEndPt);
    if (needScroll) {
        ensureVisible(matchEndPt.x(), matchEndPt.y());
        ensureVisible(matchStartPt.x(), matchStartPt.y());
    }
}

void SourceWidget::copy()
{
    sourceWidgetView().copy();
}

} // namespace Nav
