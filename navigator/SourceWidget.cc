#include "SourceWidget.h"

#include <QDebug>
#include <QTime>
#include <QMenu>
#include <QPainter>
#include <QLabel>
#include <QTextBlock>
#include <QScrollBar>
#include <cassert>

#include "Misc.h"
#include "File.h"
#include "Project.h"
#include "ReportRefList.h"
#include "TreeReportWindow.h"

#include "CXXSyntaxHighlighter.h"

namespace Nav {

const int kTabStopSize = 8;

// Sometimes the line spacing is smaller than the height, which makes the text
// cramped.  When this happens, use the height instead.  (I think Qt already
// does this -- look for QTextLayout and a negative leading()).
static int effectiveLineSpacing(QFontMetrics &fm)
{
    return std::max(fm.height(), fm.lineSpacing());
}

static inline QSize marginsToSize(const QMargins &margins)
{
    return QSize(margins.left() + margins.right(),
                 margins.top() + margins.bottom());
}

// Measure the size of the line after expanding tab stops.
static int measureLineLength(const QString &data, int start, int size, int tabStopSize)
{
    int pos = 0;
    for (int i = start; i < start + size; ++i) {
        if (data[i].unicode() == '\t')
            pos = (pos + tabStopSize) / tabStopSize * tabStopSize;
        else
            pos++;
    }
    return pos;
}


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
    m_file(NULL)
{
    setBackgroundRole(QPalette::NoRole);
}

void SourceWidgetView::setFile(File *file)
{
    m_file = file;
    m_syntaxColoring.clear();
    m_maxLineLength = 0;

    if (m_file != NULL) {
        m_syntaxColoring = CXXSyntaxHighlighter::highlight(m_file->content());

        // Measure the longest line.
        int maxLength = 0;
        const QString &content = m_file->content();
        for (int i = 0, lineCount = m_file->lineCount(); i < lineCount; ++i) {
            maxLength = std::max(
                        maxLength,
                        measureLineLength(content,
                                          m_file->lineStart(i),
                                          m_file->lineLength(i),
                                          kTabStopSize));
        }

        m_maxLineLength = maxLength;
    }

    updateGeometry();
    update();
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
        return Qt::black;
    }
}

void SourceWidgetView::paintEvent(QPaintEvent *event)
{
    if (m_file != NULL) {
        QFontMetrics fontMetrics(font());
        const int firstBaseline = fontMetrics.ascent() + m_margins.top();
        const int lineSpacing = effectiveLineSpacing(fontMetrics);
        QPainter painter(this);
        int line1 = std::max(event->rect().y() / lineSpacing - 2, 0);
        int line2 = std::min(event->rect().bottom() / lineSpacing + 2, m_file->lineCount() - 1);
        // XXX: Consider restricting the painting horizontally too.
        // XXX: Support characters outside the Unicode BMP (i.e. UTF-16 surrogate pairs).
        Qt::GlobalColor currentColor = Qt::black;
        painter.setPen(QColor(currentColor));
        int tabStopPx = fontMetrics.width(' ') * kTabStopSize;

        for (int line = line1; line <= line2; ++line) {
            int lineStart = m_file->lineStart(line);
            QString lineContent = m_file->lineContent(line).toString();
            int x = 0;
            QString oneChar(QChar('\0'));
            for (int column = 0; column < lineContent.size(); ++column) {
                oneChar[0] = lineContent[column];
                CXXSyntaxHighlighter::Kind kind = m_syntaxColoring[lineStart + column];
                Qt::GlobalColor color = colorForSyntaxKind(kind);
                if (color != currentColor) {
                    painter.setPen(QColor(color));
                    currentColor = color;
                }
                if (oneChar[0] == '\t') {
                    x = (x + tabStopPx) / tabStopPx * tabStopPx;
                } else {
                    painter.drawText(
                                x + m_margins.left(),
                                firstBaseline + line * lineSpacing,
                                oneChar);
                    x += fontMetrics.width(oneChar[0]);
                }
            }
        }
    }

    QWidget::paintEvent(event);
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


///////////////////////////////////////////////////////////////////////////////
// SourceWidget

SourceWidget::SourceWidget(Project &project, QWidget *parent) :
    QScrollArea(parent),
    m_project(project)
{
    setWidgetResizable(true);
    setWidget(new SourceWidgetView(QMargins(4, 4, 4, 4), project));
    setBackgroundRole(QPalette::Base);
    setViewportMargins(30, 0, 0, 0);
    m_lineArea = new SourceWidgetLineArea(QMargins(4, 5, 4, 4), this);
    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(scrollBarValueChanged()));

    // Configure the widgets to use a small monospace font.  Force characters
    // to have an integral width for simplicity.
    QFont font;
    font.setFamily("Monospace");
    font.setPointSize(8);
    font.setStyleStrategy(QFont::ForceIntegerMetrics);
    widget()->setFont(font);
    m_lineArea->setFont(font);

    layoutLineArea();

#if 0
    setWordWrapMode(QTextOption::NoWrap);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setTextInteractionFlags(Qt::NoTextInteraction);
    setReadOnly(true);
#endif
}

void SourceWidget::setFile(File *file)
{
    if (this->file() != file) {
        sourceWidgetView().setFile(file);
        m_lineArea->setLineCount(file != NULL ? file->lineCount() : 0);
        verticalScrollBar()->setValue(0);
        layoutLineArea();
    }
}

SourceWidgetView &SourceWidget::sourceWidgetView()
{
    return *qobject_cast<SourceWidgetView*>(widget());
}

#if 0
QSize SourceWidget::lineAreaSizeHint()
{
    if (file() == NULL)
        return QSize();
    QFontMetrics fontMetrics(m_lineArea->font());
    int lineCount = file()->lineCount();
    int digits = QString::number(lineCount).length();
    return QSize(fontMetrics.width('9') * digits,
                 lineCount * fontMetrics.height() +
                 (lineCount - 1) * (fontMetrics.leading() + kExtraLineSpacingPx) +
                 kMarginPx * 2);
}

void SourceWidget::lineAreaPaintEvent(QPaintEvent *event)
{
    if (file() != NULL) {
        QFontMetrics metrics(m_lineArea->font());
        int lineCount = file()->lineCount();
        QPainter painter(m_lineArea);
        for (int i = 0; i < lineCount; ++i) {
            painter.drawText(0,
                             20 +      i * (metrics.lineSpacing() + kExtraLineSpacingPx),
                             QString::number(i + 1));
        }
    }
}
#endif

void SourceWidget::layoutLineArea(void)
{
    QSize lineAreaSizeHint = m_lineArea->sizeHint();
    m_lineArea->setGeometry(
                0,
                -verticalScrollBar()->value(),
                lineAreaSizeHint.width(),
                lineAreaSizeHint.height());
    setViewportMargins(lineAreaSizeHint.width(), 0, 0, 0);
}

void SourceWidget::scrollBarValueChanged()
{
    layoutLineArea();
}

#if 0
void SourceWidget::mousePressEvent(QMouseEvent *event)
{
    QTextCursor cursor = cursorForPosition(event->pos());
    cursor = findEnclosingIdentifier(cursor);
    setTextCursor(cursor);
}

void SourceWidget::clearSelection()
{
    QTextCursor cursor = textCursor();
    cursor.clearSelection();
    setTextCursor(cursor);
}

void SourceWidget::mouseReleaseEvent(QMouseEvent *event)
{
    QTextCursor cursor = cursorForPosition(event->pos());
    cursor = findEnclosingIdentifier(cursor);
    if (cursor == textCursor()) {

        // The identifier was clicked..
        // TODO: Go to the definition of the identifier.

    }
    clearSelection();
}

void SourceWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
}

void SourceWidget::mouseMoveEvent(QMouseEvent *event)
{
    QTextCursor cursor = cursorForPosition(event->pos());
    cursor = findEnclosingIdentifier(cursor);
    if (cursor != textCursor())
        clearSelection();
}

void SourceWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QTextCursor cursor = cursorForPosition(event->pos());
    cursor = findEnclosingIdentifier(cursor);
    if (!cursor.selectedText().isEmpty()) {
        QMenu *menu = new QMenu();
        int line = cursor.blockNumber() + 1;
        int column = cursor.selectionStart() - cursor.block().position() + 1;
        QStringList symbols = theProject->querySymbolsAtLocation(m_file, line, column);
        if (symbols.isEmpty()) {
            QAction *action = menu->addAction("No symbols found");
            action->setEnabled(false);
        } else {
            for (QString symbol : symbols) {
                QAction *action = menu->addAction(symbol);
                action->setEnabled(false);
                //action->setSeparator(true);
                if (1) {
                    QFont f = action->font();
                    f.setBold(true);
                    action->setFont(f);
                }
                menu->addSeparator();
                action = menu->addAction("Cross-references...", this, SLOT(actionCrossReferences()));
                action->setData(symbol);
                menu->addSeparator();
            }
        }
        menu->exec(event->globalPos());
        delete menu;
    }
    clearSelection();
}

void SourceWidget::actionCrossReferences()
{
    QAction *action = qobject_cast<QAction*>(sender());
    QString symbol = action->data().toString();
    ReportRefList *r = new ReportRefList(theProject, symbol);
    TreeReportWindow *tw = new TreeReportWindow(r);
    tw->show();
}

static bool isIdentifierChar(QChar ch)
{
    return ch.isLetterOrNumber() || ch == '_';
}

QTextCursor SourceWidget::findEnclosingIdentifier(QTextCursor pt)
{
    QString lineContent = pt.block().text();
    int first = pt.positionInBlock();
    int last = pt.positionInBlock();

    if (first >= lineContent.size())
        return pt;

    if (isIdentifierChar(lineContent[first])) {
        while (first - 1 >= 0 && isIdentifierChar(lineContent[first - 1]))
            first--;
        while (last + 1 < lineContent.size() && isIdentifierChar(lineContent[last + 1]))
            last++;
    }

    pt.setPosition(pt.block().position() + first);
    pt.setPosition(pt.block().position() + last + 1, QTextCursor::KeepAnchor);
    return pt;
}
#endif

// Line and column indices are 1-based.
void SourceWidget::selectIdentifier(int line, int column)
{
#if 0
    // TODO: Do tab stops affect the column?
    QTextCursor c(document());
    c.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, line - 1);
    c.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column - 1);
    c = findEnclosingIdentifier(c);
    setTextCursor(c);
#endif
}

} // namespace Nav
