#include "SourceWidget.h"

#include <QApplication>
#include <QBrush>
#include <QClipboard>
#include <QColor>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QFontMetricsF>
#include <QMargins>
#include <QMenu>
#include <QMoveEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPoint>
#include <QLabel>
#include <QTextBlock>
#include <QRect>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSize>
#include <QString>
#include <QWidget>
#include <cassert>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <re2/prog.h>
#include <re2/re2.h>
#include <re2/regexp.h>

#include "Application.h"
#include "CXXSyntaxHighlighter.h"
#include "File.h"
#include "Misc.h"
#include "Project.h"
#include "Project-inl.h"
#include "Ref.h"
#include "Regex.h"
#include "RegexMatchList.h"
#include "ReportRefList.h"
#include "StringRef.h"
#include "TableReportWindow.h"
#include "MainWindow.h"

namespace Nav {


///////////////////////////////////////////////////////////////////////////////
// Miscellaneous

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

static int measureLongestLine(File &file, int tabStopSize)
{
    int ret = 0;
    for (int i = 0, lineCount = file.lineCount(); i < lineCount; ++i) {
        ret = std::max(
                    ret,
                    measureLineLength(file.lineContent(i), tabStopSize));
    }
    return ret;
}

namespace {

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
            int line,
            int tabStopSize) :
        m_twc(TextWidthCalculator::getCachedTextWidthCalculator(font)),
        m_fm(font)
    {
        m_lineContent = file.lineContent(line);
        m_lineHeight = effectiveLineSpacing(m_fm);
        m_lineTop = margins.top() + line * m_lineHeight;
        m_lineBaselineY = m_lineTop + m_fm.ascent();
        m_lineLeftMargin = margins.left();
        m_lineStartIndex = file.lineStart(line);
        m_tabStopPx = m_twc.calculate(" ") * tabStopSize;
        m_charIndex = -1;
        m_charNextIndex = 0;
        m_charLeft = 0;
        m_charWidth = 0;
    }

    bool hasMoreChars()
    {
        return m_charNextIndex < static_cast<int>(m_lineContent.size());
    }

    void advanceChar()
    {
        assert(hasMoreChars());
        // Skip to the next character.
        m_charIndex = m_charNextIndex;
        const int len = charByteLen();
        m_charNextIndex = m_charIndex + len;
        m_charLeft += m_charWidth;
        // Update the current character's text and width properties.
        const char *const pch = &m_lineContent[m_charIndex];
        if (*pch == '\t') {
            m_charText.clear();
            const int tabStopIndex = (m_charLeft + m_tabStopPx) / m_tabStopPx;
            m_charWidth = tabStopIndex * m_tabStopPx - m_charLeft;
        } else {
            m_charText.resize(len);
            for (int i = 0; i < len; ++i)
                m_charText[i] = pch[i];
            m_charWidth = m_twc.calculate(m_charText.c_str());
        }
    }

    int charColumn()                { return m_charIndex; }
    int charFileIndex()             { return m_lineStartIndex + m_charIndex; }
    qreal charLeft()                { return m_lineLeftMargin + m_charLeft; }
    qreal charWidth()               { return m_charWidth; }
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

private:
    TextWidthCalculator m_twc;
    QFontMetrics m_fm;
    StringRef m_lineContent;
    int m_lineHeight;
    int m_lineTop;
    int m_lineBaselineY;
    int m_lineLeftMargin;
    int m_lineStartIndex;
    qreal m_tabStopPx;
    int m_charIndex;
    int m_charNextIndex;
    qreal m_charLeft;
    qreal m_charWidth;
    std::string m_charText;
};


///////////////////////////////////////////////////////////////////////////////
// <anon>::LineTextPainter

class LineTextPainter {
public:
    LineTextPainter(
            QPainter &painter,
            SourceWidgetTextPalette &textPalette,
            const QFont &font,
            int y);
    void drawChar(
            qreal x,
            const std::string &ch,
            SourceWidgetTextPalette::Color color);
    void flushLine();

private:
    struct ColoredPart {
        QString text;
        qreal x1;
        qreal x2;
    };

    struct ColoredLine {
        SourceWidgetTextPalette::Color color;
        std::vector<ColoredPart> parts;
    };

    inline ColoredLine &coloredLine(SourceWidgetTextPalette::Color color);

    QPainter &m_painter;
    SourceWidgetTextPalette &m_textPalette;
    int m_y;
    TextWidthCalculator m_twc;
    qreal m_spaceCharWidth;
    std::map<SourceWidgetTextPalette::Color, std::unique_ptr<ColoredLine> >
            m_coloredLines;
    ColoredLine *m_recentColoredLine;
};

LineTextPainter::LineTextPainter(
        QPainter &painter,
        SourceWidgetTextPalette &textPalette,
        const QFont &font,
        int y) :
    m_painter(painter),
    m_textPalette(textPalette),
    m_y(y),
    m_twc(TextWidthCalculator::getCachedTextWidthCalculator(font)),
    m_spaceCharWidth(m_twc.calculate(" ")),
    m_recentColoredLine(NULL)
{
    assert(!font.kerning());
}

void LineTextPainter::drawChar(
        qreal x,
        const std::string &ch,
        SourceWidgetTextPalette::Color color)
{
    ColoredLine &cline = coloredLine(color);
    if (ch.size() != 1 || ch[0] < 32 || ch[0] > 126) {
        m_painter.setPen(m_textPalette.pen(color));
        m_painter.drawText(QPointF(x, m_y), QString::fromStdString(ch));
        return;
    }
    if (!cline.parts.empty()) {
        ColoredPart &part = cline.parts.back();
        if (part.x2 <= x) {
            if (x > part.x2) {
                qreal spaceCountF = (x - part.x2) / m_spaceCharWidth;
                int spaceCount = static_cast<int>(spaceCountF);
                if (spaceCount == spaceCountF) {
                    part.text.append(QString(spaceCount, ' '));
                    part.x2 += m_spaceCharWidth * spaceCount;
                }
            }
            if (x == part.x2) {
                part.text.append(ch[0]);
                part.x2 += m_twc.calculate(ch.c_str());
                return;
            }
        }
    }
    ColoredPart newPart;
    newPart.text = QString::fromStdString(ch);
    newPart.x1 = x;
    newPart.x2 = x + m_twc.calculate(ch.c_str());
    cline.parts.push_back(newPart);
}

void LineTextPainter::flushLine()
{
    for (const auto &it : m_coloredLines) {
        ColoredLine &cline = *it.second;
        m_painter.setPen(m_textPalette.pen(cline.color));
        for (const ColoredPart &part : cline.parts)
            m_painter.drawText(QPointF(part.x1, m_y), part.text);
    }
}

inline LineTextPainter::ColoredLine &LineTextPainter::coloredLine(
        SourceWidgetTextPalette::Color color)
{
    if (m_recentColoredLine != NULL && m_recentColoredLine->color == color)
        return *m_recentColoredLine;
    auto it = m_coloredLines.find(color);
    ColoredLine *ret;
    if (it != m_coloredLines.end()) {
        ret = it->second.get();
    } else {
        m_coloredLines[color] =
                std::unique_ptr<ColoredLine>(ret = new ColoredLine);
        ret->color = color;
    }
    m_recentColoredLine = ret;
    return *ret;
}

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
    const int kLineBleedPx = ch / 2 + 1; // a guess
    const QRegion virtualRegion = event->region().translated(m_viewportOrigin);
    const int line1 = std::max(
                (virtualRegion.boundingRect().top() -
                        m_margins.top() - kLineBleedPx) / ch,
                0);
    const int line2 = std::min(
                (virtualRegion.boundingRect().bottom() -
                        m_margins.top() + kLineBleedPx) / ch,
                m_lineCount - 1);
    const int thisWidth = width();

    p.setPen(QColor(Qt::darkGray));
    for (int line = line1; line <= line2; ++line) {
        QString text = QString::number(line + 1);
        p.drawText(thisWidth - (text.size() * cw) - m_margins.right() -
                        m_viewportOrigin.x(),
                   ch * line + ca + m_margins.top() -
                        m_viewportOrigin.y(),
                   text);
    }
}

void SourceWidgetLineArea::setViewportOrigin(QPoint pt)
{
    if (pt == m_viewportOrigin)
        return;
    QPoint oldPt = m_viewportOrigin;
    scroll(oldPt.x() - pt.x(), oldPt.y() - pt.y());
    m_viewportOrigin = pt;
}


///////////////////////////////////////////////////////////////////////////////
// SourceWidgetTextPalette

SourceWidgetTextPalette::SourceWidgetTextPalette(Project &project) :
    m_project(project)
{
    m_pens.resize(static_cast<size_t>(Color::specialColorCount));
    m_pens[static_cast<size_t>(Color::transparent)].setColor(Qt::transparent);

    m_syntaxColor.resize(CXXSyntaxHighlighter::KindMax, Color::defaultText);
    setSyntaxColor(CXXSyntaxHighlighter::KindComment, Qt::darkGreen);
    setSyntaxColor(CXXSyntaxHighlighter::KindQuoted, Qt::darkGreen);
    setSyntaxColor(CXXSyntaxHighlighter::KindNumber, Qt::darkBlue);
    setSyntaxColor(CXXSyntaxHighlighter::KindDirective, Qt::darkBlue);
    setSyntaxColor(CXXSyntaxHighlighter::KindKeyword, Qt::darkYellow);

    setSymbolTypeColor("GlobalVariable", Qt::darkCyan);
    setSymbolTypeColor("Field", Qt::darkRed);
    setSymbolTypeColor("Namespace", Qt::darkMagenta);
    setSymbolTypeColor("Struct", Qt::darkMagenta);
    setSymbolTypeColor("Class", Qt::darkMagenta);
    setSymbolTypeColor("Union", Qt::darkMagenta);
    setSymbolTypeColor("Enum", Qt::darkMagenta);
    setSymbolTypeColor("Typedef", Qt::darkMagenta);
    setSymbolTypeColor("Macro", Qt::darkBlue);
}

void SourceWidgetTextPalette::setDefaultTextColor(const QColor &color)
{
    m_pens[static_cast<size_t>(Color::defaultText)].setColor(color);
}

void SourceWidgetTextPalette::setHighlightedTextColor(const QColor &color)
{
    m_pens[static_cast<size_t>(Color::highlightedText)].setColor(color);
}

SourceWidgetTextPalette::Color SourceWidgetTextPalette::colorForSyntaxKind(
        CXXSyntaxHighlighter::Kind kind) const
{
    assert(kind < m_syntaxColor.size());
    return m_syntaxColor[kind];
}

SourceWidgetTextPalette::Color SourceWidgetTextPalette::colorForRef(
        const Ref &ref) const
{
    auto it = m_symbolTypeColor.find(
                m_project.querySymbolType(ref.symbolID()));
    return it == m_symbolTypeColor.end() ? Color::transparent : it->second;
}

const QPen &SourceWidgetTextPalette::pen(Color color) const
{
    size_t colorIndex = static_cast<size_t>(color);
    assert(colorIndex < m_pens.size());
    return m_pens[colorIndex];
}

void SourceWidgetTextPalette::setSyntaxColor(
        CXXSyntaxHighlighter::Kind kind,
        const QColor &color)
{
    assert(kind < m_syntaxColor.size());
    m_syntaxColor[kind] = registerColor(color);
}

void SourceWidgetTextPalette::setSymbolTypeColor(
        const char *symbolType,
        const QColor &color)
{
    Color c = registerColor(color);
    indexdb::ID symbolTypeID = m_project.getSymbolTypeID(symbolType);
    if (symbolTypeID != indexdb::kInvalidID)
        m_symbolTypeColor[symbolTypeID] = c;
}

SourceWidgetTextPalette::Color SourceWidgetTextPalette::registerColor(
        const QColor &color)
{
    for (size_t i = static_cast<size_t>(Color::specialColorCount);
            i < m_pens.size(); ++i) {
        if (m_pens[i].color() == color)
            return static_cast<Color>(i);
    }
    QPen newPen(color);
    m_pens.push_back(newPen);
    Color newColor = static_cast<Color>(m_pens.size() - 1);
    assert(static_cast<size_t>(newColor) == m_pens.size() - 1 &&
           "Too many colors in the source widget's text palette");
    return newColor;
}


///////////////////////////////////////////////////////////////////////////////
// SourceWidgetView

SourceWidgetView::SourceWidgetView(
        const QMargins &margins,
        Project &project,
        QWidget *parent) :
    QWidget(parent),
    m_textPalette(project),
    m_margins(margins),
    m_project(project),
    m_file(NULL),
    m_mouseHoveringInWidget(false),
    m_selectingMode(SM_Inactive),
    m_selectedMatchIndex(-1),
    m_tabStopSize(8)
{
    setAutoFillBackground(true);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);
    updateFindMatches();
}

// Declare out-of-line destructor where member std::unique_ptr's types are
// defined.
SourceWidgetView::~SourceWidgetView()
{
}

void SourceWidgetView::setViewportOrigin(QPoint pt)
{
    if (pt == m_viewportOrigin)
        return;
    QPoint oldPt = m_viewportOrigin;
    scroll(oldPt.x() - pt.x(), oldPt.y() - pt.y());
    m_viewportOrigin = pt;
    updateSelectionAndHover();
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
        m_syntaxColoring = std::unique_ptr<SourceWidgetTextPalette::Color[]>(
                    new SourceWidgetTextPalette::Color[content.size()]);
        for (size_t i = 0; i < content.size(); ++i) {
            m_syntaxColoring[i] =
                    m_textPalette.colorForSyntaxKind(syntaxColoringKind[i]);
        }

        // Color characters according to the index's refs.
        m_project.queryFileRefs(*m_file, [=](const Ref &ref) {
            if (ref.line() > m_file->lineCount())
                return;
            int offset = m_file->lineStart(ref.line() - 1);
            auto color = m_textPalette.colorForRef(ref);
            if (color != SourceWidgetTextPalette::Color::transparent) {
                for (int i = ref.column() - 1,
                        iEnd = std::min(ref.endColumn() - 1,
                                        m_file->lineLength(ref.line() - 1));
                        i < iEnd; ++i) {
                    m_syntaxColoring[offset + i] = color;
                }
            }
        });

        // Measure the longest line.
        m_maxLineLength = measureLongestLine(*m_file, m_tabStopSize);
    }

    updateFindMatches();
    updateGeometry();
    update();
}

void SourceWidgetView::paintEvent(QPaintEvent *event)
{
    if (m_file == NULL)
        return;

    m_textPalette.setDefaultTextColor(
                palette().color(foregroundRole()));
    m_textPalette.setHighlightedTextColor(
                palette().color(QPalette::HighlightedText));

    const int lineSpacing = effectiveLineSpacing(fontMetrics());
    QPainter painter(this);

    // Paint lines in the clip region.
    const int kLineBleedPx = lineSpacing / 2 + 1; // a guess
    const QRegion virtualRegion = event->region().translated(m_viewportOrigin);
    const int line1 = std::max(
                (virtualRegion.boundingRect().top() -
                        m_margins.top() - kLineBleedPx) /
                        lineSpacing,
                0);
    const int line2 = std::min(
                (virtualRegion.boundingRect().bottom() -
                        m_margins.top() + kLineBleedPx) /
                        lineSpacing,
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
        paintLine(painter, line, virtualRegion, findMatch);
}

int SourceWidgetView::lineTop(int line)
{
    return effectiveLineSpacing(fontMetrics()) * line + m_margins.top();
}

// line is 0-based.
void SourceWidgetView::paintLine(
        QPainter &painter,
        int line,
        const QRegion &paintRegion,
        RegexMatchList::iterator &findMatch)
{
    const int selStartOff = m_selectedRange.start.toOffset(*m_file);
    const int selEndOff = m_selectedRange.end.toOffset(*m_file);
    const int hovStartOff = m_hoverHighlightRange.start.toOffset(*m_file);
    const int hovEndOff = m_hoverHighlightRange.end.toOffset(*m_file);
    const QBrush matchBrush(Qt::yellow);
    const QBrush selectedMatchBrush(QColor(255, 140, 0));
    const QBrush hoverBrush(QColor(200, 200, 200));
    const int rightEdge = paintRegion.boundingRect().right() + 1;

    // Fill the line's background.
    {
        LineLayout lay(font(), m_margins, *m_file, line, m_tabStopSize);
        QRect charBox(0, lay.lineTop(), 0, lay.lineHeight());
        while (lay.hasMoreChars()) {
            lay.advanceChar();
            if (lay.charLeft() >= rightEdge)
                break;
            charBox.setLeft(qRound(lay.charLeft()));
            charBox.setRight(qRound(lay.charLeft() + lay.charWidth()) - 1);
            if (!paintRegion.intersects(charBox))
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
                painter.fillRect(charBox.translated(-m_viewportOrigin),
                                 *fillBrush);
            }
        }
    }

    // Draw characters.
    {
        LineLayout lay(font(), m_margins, *m_file, line, m_tabStopSize);
        LineTextPainter lineTextPainter(
                    painter,
                    m_textPalette,
                    font(),
                    lay.lineBaselineY() - m_viewportOrigin.y());
        TextWidthCalculator &twc =
                TextWidthCalculator::getCachedTextWidthCalculator(font());
        const int kLineBleedPx = lay.lineHeight() / 2 + 1; // a guess
        const qreal horizBleedPx =
                std::max<qreal>(
                    twc.calculate(" "),
                    std::max(
                        -twc.minLeftBearing(),
                        -twc.minRightBearing()));
        QRect charBox(0, lay.lineTop() - kLineBleedPx,
                      0, lay.lineHeight() + kLineBleedPx * 2);
        while (lay.hasMoreChars()) {
            lay.advanceChar();

            // Truncate qreal to int coordinates in charBox.  The width is a
            // conservative overestimate.
            charBox.setLeft(lay.charLeft() - horizBleedPx);
            charBox.setWidth(lay.charWidth() + horizBleedPx * 2 + 2);
            if (!paintRegion.intersects(charBox))
                continue;
            if (charBox.left() >= rightEdge)
                break;

            if (!lay.charText().empty()) {
                FileLocation loc(line, lay.charColumn());
                SourceWidgetTextPalette::Color color =
                        m_syntaxColoring[lay.charFileIndex()];

                // Override the color for selected text.
                if (loc >= m_selectedRange.start && loc < m_selectedRange.end)
                    color = SourceWidgetTextPalette::Color::highlightedText;

                lineTextPainter.drawChar(
                            lay.charLeft() - m_viewportOrigin.x(),
                            lay.charText(),
                            color);
            }
        }
        lineTextPainter.flushLine();
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
        LineLayout lay(font(), m_margins, *m_file, line, m_tabStopSize);
        while (lay.hasMoreChars()) {
            lay.advanceChar();
            qreal charWidth = lay.charWidth();
            if (roundToNearest)
                charWidth = charWidth / 2;
            if (pixel.x() < qRound(lay.charLeft() + charWidth))
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
    LineLayout lay(font(), m_margins, *m_file, loc.line, m_tabStopSize);
    while (lay.hasMoreChars()) {
        lay.advanceChar();
        if (lay.charColumn() == loc.column)
            return QPoint(qRound(lay.charLeft()), lineTop(loc.line));
    }
    return QPoint(qRound(lay.charLeft() + lay.charWidth()), lineTop(loc.line));
}

QSize SourceWidgetView::sizeHint() const
{
    if (m_file == NULL)
        return marginsToSize(m_margins);
    QFontMetrics fontMetrics(font());
    QFontMetricsF fontMetricsF(font());
    return QSize(m_maxLineLength * fontMetricsF.width(' ') + 1,
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

int SourceWidgetView::tabStopSize()
{
    return m_tabStopSize;
}

void SourceWidgetView::setTabStopSize(int size)
{
    if (m_tabStopSize == size)
        return;
    m_tabStopSize = size;
    if (m_file != NULL)
        m_maxLineLength = measureLongestLine(*m_file, m_tabStopSize);
    update();
}

void SourceWidgetView::copy()
{
    StringRef text = rangeText(m_selectedRange);
    if (text.size() > 0) {
        QString qtext = QString::fromUtf8(text.data(), text.size());
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
        QPoint pt1 = locationToPoint(range.start) - m_viewportOrigin;
        QPoint pt2 = locationToPoint(range.end) - m_viewportOrigin;
        update(pt1.x(), pt1.y(), pt2.x() - pt1.x(), lineHeight);
    } else {
        int y1 = lineTop(range.start.line) - m_viewportOrigin.y();
        int y2 = lineTop(range.end.line) + lineHeight - m_viewportOrigin.y();
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

bool SourceWidgetView::handleBackForwardMouseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::XButton1) {
        emit goBack();
        return true;
    } else if (event->button() == Qt::XButton2) {
        emit goForward();
        return true;
    }
    return false;
}

void SourceWidgetView::mousePressEvent(QMouseEvent *event)
{
    if (handleBackForwardMouseEvent(event)) {
        return;
    }
    const QPoint virtualPos = event->pos() + m_viewportOrigin;
    if (m_tripleClickTime.elapsed() < QApplication::doubleClickInterval() &&
            (virtualPos - m_tripleClickPoint).manhattanLength() <
                QApplication::startDragDistance()) {
        m_tripleClickTime = QTime();
        m_tripleClickPoint = QPoint();
        navMouseTripleDownEvent(event, virtualPos);
    } else {
        navMouseSingleDownEvent(event, virtualPos);
    }
}

void SourceWidgetView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (handleBackForwardMouseEvent(event)) {
        return;
    }
    const QPoint virtualPos = event->pos() + m_viewportOrigin;
    m_tripleClickTime.start();
    m_tripleClickPoint = virtualPos;
    navMouseDoubleDownEvent(event, virtualPos);
}

void SourceWidgetView::navMouseSingleDownEvent(
        QMouseEvent *event,
        QPoint virtualPos)
{
    FileLocation loc = hitTest(virtualPos);
    FileRange refRange = findRefAtLocation(loc);
    if (!refRange.isEmpty()) {
        m_selectingMode = SM_Ref;
        setSelection(refRange);
    } else {
        if (event->button() == Qt::LeftButton) {
            m_selectingMode = SM_Char;
            m_selectingAnchor = virtualPos;
        } else if (event->button() == Qt::RightButton &&
                !m_selectedRange.isEmpty() &&
                (loc < m_selectedRange.start || loc >= m_selectedRange.end)) {
            setSelection(FileRange());
        }
    }
    updateSelectionAndHover(virtualPos);
}

void SourceWidgetView::navMouseDoubleDownEvent(
        QMouseEvent *event,
        QPoint virtualPos)
{
    if (event->button() == Qt::LeftButton) {
        m_selectingMode = SM_Word;
        m_selectingAnchor = virtualPos;
        updateSelectionAndHover(virtualPos);
    }
}

void SourceWidgetView::navMouseTripleDownEvent(
        QMouseEvent *event,
        QPoint virtualPos)
{
    if (event->button() == Qt::LeftButton) {
        m_selectingMode = SM_Line;
        m_selectingAnchor = virtualPos;
        updateSelectionAndHover(virtualPos);
    }
}

void SourceWidgetView::mouseMoveEvent(QMouseEvent *event)
{
    const QPoint virtualPos = event->pos() + m_viewportOrigin;
    updateSelectionAndHover(virtualPos);
    if (m_selectingMode != SM_Inactive && m_selectingMode != SM_Ref)
        emit pointSelected(virtualPos);
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
    updateSelectionAndHover(mapFromGlobal(QCursor::pos()) + m_viewportOrigin);
}

// Update the selected range, hover highlight range, and mouse cursor to
// account for a mouse move to the given position.
void SourceWidgetView::updateSelectionAndHover(QPoint virtualPos)
{
    if (m_file == NULL)
        return;
    if (m_selectingMode == SM_Inactive) {
        FileRange word;
        const QRect virtualRect = rect().translated(m_viewportOrigin);
        if (virtualRect.contains(virtualPos) && m_mouseHoveringInWidget)
            word = findRefAtPoint(virtualPos);
        setHoverHighlight(word);
        setCursor(word.isEmpty() ? Qt::ArrowCursor : Qt::PointingHandCursor);
        m_selectingAnchor = QPoint();
    } else if (m_selectingMode == SM_Ref) {
        setHoverHighlight(FileRange());
        if (!m_selectedRange.isEmpty()) {
            FileRange word = findRefAtPoint(virtualPos);
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
            FileLocation loc2 = hitTest(virtualPos,
                                        /*roundToNearest=*/true);
            setSelection(FileRange(std::min(loc1, loc2),
                                   std::max(loc1, loc2)));
        } else if (m_selectingMode == SM_Word) {
            setCursor(Qt::ArrowCursor);
            FileRange loc1 = findWordAtLocation(hitTest(m_selectingAnchor));
            FileRange loc2 = findWordAtLocation(hitTest(virtualPos));
            setSelection(FileRange(std::min(loc1.start, loc2.start),
                                   std::max(loc1.end, loc2.end)));
        } else if (m_selectingMode == SM_Line) {
            setCursor(Qt::ArrowCursor);
            FileLocation loc1 = hitTest(m_selectingAnchor);
            FileLocation loc2 = hitTest(virtualPos);
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
            QString qtext = QString::fromUtf8(text.data(), text.size());
            QApplication::clipboard()->setText(qtext, QClipboard::Selection);
        }
    }
}

void SourceWidgetView::mouseReleaseEvent(QMouseEvent *event)
{
    const QPoint virtualPos = event->pos() + m_viewportOrigin;
    SelectingMode oldMode = m_selectingMode;
    m_selectingMode = SM_Inactive;
    updateSelectionAndHover(virtualPos);

    if (oldMode == SM_Ref && !m_selectedRange.isEmpty()) {
        FileRange identifierClicked;
        if (m_selectedRange == findRefAtPoint(virtualPos))
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
    QAbstractScrollArea(parent),
    m_project(project),
    m_findStartOffset(0)
{
#if NAV_MACSCROLLOPTIMIZATION_HACK
    m_macScrollOptimizationHack = new QWidget(this);
    m_macScrollOptimizationHack->setGeometry(0, 0, 0, 0);
    assert(m_macScrollOptimizationHack->size() == QSize(0, 0));
#endif

    m_view = new SourceWidgetView(QMargins(4, 4, 4, 4), project, viewport());
    setBackgroundRole(QPalette::Base);
    m_lineArea = new SourceWidgetLineArea(QMargins(4, 4, 4, 4), this);

    // Configure the widgets to use a small monospace font.
    QFont font = Application::instance()->sourceFont();
    font.setKerning(false);
    setFont(font);
    m_view->setFont(font);
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
    return *m_view;
}

void SourceWidget::scrollContentsBy(int dx, int dy)
{
#if NAV_MACSCROLLOPTIMIZATION_HACK
    // Work around QTBUG-29220, a scrolling bug affecting Qt4 on Cocoa.  This
    // workaround creates and scrolls a dummy widget of size (0, 0).  It is
    // important that the widget's isVisible() be true, even though there are
    // no visible pixels for it.  If isVisible() were false, the scroll call
    // would do nothing.  The scroll call should indirectly call
    // QMacScrollOptimization::delayScroll, which will set
    // QMacScrollOptimization::_target to the dummy widget, unless some other
    // widget has already been scrolled.  The effect is to disable
    // QMacScrollOptimization for the source widget.  See the Qt bug report for
    // details.
    m_macScrollOptimizationHack->scroll(1, 1);
#endif
    m_lineArea->setViewportOrigin(QPoint(0, viewportOrigin().y()));
    m_view->setViewportOrigin(viewportOrigin());
}

// TODO: Replace this with QStyle hint calls.
const int kViewportScrollbarMargin = 3;

// It's better if this size is an underestimate rather than an overestimate.
// It takes the current scrollbar status into account, unlike
// viewport()->size(), which uses layout information cached somewhere and
// only incorporates scrollbar status after some layout process completes.
QSize SourceWidget::estimatedViewportSize()
{
    bool vbar = false;
    bool hbar = false;
    int viewportHeightEstimate = height() - frameWidth() * 2;
    int viewportWidthEstimate = width() - frameWidth() * 2 -
            m_lineArea->width();
    const int barW = verticalScrollBar()->sizeHint().width() +
            kViewportScrollbarMargin;
    const int barH = horizontalScrollBar()->sizeHint().height() +
            kViewportScrollbarMargin;

    const QSize contentSize = m_view->sizeHint();
    for (int round = 0; round < 2; ++round) {
        if (!vbar && viewportHeightEstimate < contentSize.height()) {
            vbar = true;
            viewportWidthEstimate -= barW;
        }
        if (!hbar && viewportWidthEstimate < contentSize.width()) {
            hbar = true;
            viewportHeightEstimate -= barH;
        }
    }

    return QSize(viewportWidthEstimate, viewportHeightEstimate);
}

void SourceWidget::updateScrollBars()
{
    const int lineHeight = effectiveLineSpacing(fontMetrics());
    const QSize contentSize = m_view->sizeHint();
    const QSize viewportSize = estimatedViewportSize();
    verticalScrollBar()->setRange(
                0, contentSize.height() - viewportSize.height());
    verticalScrollBar()->setPageStep(
                std::max(lineHeight, viewportSize.height() - lineHeight));
    verticalScrollBar()->setSingleStep(lineHeight);

    horizontalScrollBar()->setRange(
                0, contentSize.width() - viewportSize.width());
    horizontalScrollBar()->setPageStep(viewportSize.width());
    horizontalScrollBar()->setSingleStep(
                fontMetrics().averageCharWidth() * 20);
}

void SourceWidget::layoutSourceWidget(void)
{
    QSize lineAreaSizeHint = m_lineArea->sizeHint();
    setViewportMargins(lineAreaSizeHint.width(), 0, 0, 0);
    m_lineArea->setGeometry(
                viewport()->pos().x() - lineAreaSizeHint.width(),
                viewport()->pos().y(),
                lineAreaSizeHint.width(),
                viewport()->height());
    m_view->setGeometry(0, 0, viewport()->width(), viewport()->height());
    updateScrollBars();
}

void SourceWidget::viewPointSelected(QPoint point)
{
    ensureVisible(point, 0, 0);
}

void SourceWidget::resizeEvent(QResizeEvent *event)
{
    layoutSourceWidget();
    QAbstractScrollArea::resizeEvent(event);
}

void SourceWidget::keyPressEvent(QKeyEvent *event)
{
    bool isHome = event->key() == Qt::Key_Home;
    bool isEnd = event->key() == Qt::Key_End;

#if defined(__APPLE__)
    if (event->modifiers() & Qt::ControlModifier) {
        isHome = isHome || event->key() == Qt::Key_Up;
        isEnd = isEnd || event->key() == Qt::Key_Down;
    }
#endif

    if (isHome)
        verticalScrollBar()->setValue(0);
    else if (isEnd)
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    else
        QAbstractScrollArea::keyPressEvent(event);
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

int SourceWidget::tabStopSize()
{
    return m_view->tabStopSize();
}

void SourceWidget::setTabStopSize(int size)
{
    m_view->setTabStopSize(size);
    layoutSourceWidget();
}

// When the user starts a new search, record the viewport location so we can
// return to it if the user clears their search / backspaces through part of
// it.
void SourceWidget::recordFindStart()
{
    QPoint topLeft = viewportOrigin();
    topLeft.setY(topLeft.y() + effectiveLineSpacing(fontMetrics()) - 1);
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

// Adjust the scrollbars if necessary to ensure that the virtual coordinate is
// visible in the viewport.  This function is analogous to
// QScrollArea::ensureVisible.
void SourceWidget::ensureVisible(QPoint pt, int xMargin, int yMargin)
{
    const QSize sz = viewport()->size();
    QPoint origin = viewportOrigin();

    if (origin.y() + sz.height() - pt.y() < yMargin)
        origin.setY(pt.y() - sz.height() + yMargin);
    if (pt.y() - origin.y() < yMargin)
        origin.setY(pt.y() - yMargin);

    if (origin.x() + sz.width() - pt.x() < xMargin)
        origin.setX(pt.x() - sz.width() + xMargin);
    if (pt.x() - origin.x() < xMargin)
        origin.setX(pt.x() - xMargin);

    setViewportOrigin(origin);
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
    matchEndPt.setY(matchEndPt.y() + effectiveLineSpacing(fontMetrics()));

    QRect visibleViewRect(
                QPoint(horizontalScrollBar()->value(),
                       verticalScrollBar()->value()),
                viewport()->size());
    bool needScroll = !visibleViewRect.contains(matchStartPt) ||
                      !visibleViewRect.contains(matchEndPt);
    if (needScroll) {
        ensureVisible(matchEndPt);
        ensureVisible(matchStartPt);
    }
}

void SourceWidget::copy()
{
    sourceWidgetView().copy();
}

} // namespace Nav
