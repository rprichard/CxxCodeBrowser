#include "SourceWidget.h"

#include <QDebug>
#include <QTime>
#include <QMenu>
#include <QPainter>
#include <QLabel>
#include <QTextBlock>
#include <cassert>

#include "Misc.h"
#include "File.h"
#include "Project.h"
#include "ReportRefList.h"
#include "TreeReportWindow.h"

#include "CXXSyntaxHighlighter.h"

namespace Nav {

const int kTabStopSize = 8;
const int kMarginPx = 2;
const int kExtraLineSpacingPx = 2;


///////////////////////////////////////////////////////////////////////////////
// SourceWidgetView

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

SourceWidgetView::SourceWidgetView(Project &project, File &file) :
    m_project(project),
    m_file(file)
{
    setBackgroundRole(QPalette::NoRole);

    m_syntaxColoring = CXXSyntaxHighlighter::highlight(file.content());

    // Configure the widgets to use a small monospace font.  Force characters
    // to have an integral width for simplicity.
    QFont font;
    font.setFamily("Monospace");
    font.setPointSize(8);
    font.setStyleStrategy(QFont::ForceIntegerMetrics);
    setFont(font);
    QFontMetrics fontMetrics(font);

    // Measure the longest line.
    int maxLength = 0;
    const QString &content = file.content();
    for (int i = 0, lineCount = file.lineCount(); i < lineCount; ++i) {
        maxLength = std::max(
                    maxLength,
                    measureLineLength(content,
                                      file.lineStart(i),
                                      file.lineLength(i),
                                      kTabStopSize));
    }
    m_maxLineCount = maxLength;
}

void SourceWidgetView::paintEvent(QPaintEvent *event)
{
    QTime t;
    t.start();

    QFontMetrics fontMetrics(font());
    int lineAscent = fontMetrics.ascent() + kMarginPx;
    int lineSpacing = fontMetrics.lineSpacing() + kExtraLineSpacingPx;
    int charWidth = fontMetrics.width(' ');
    QPainter painter(this);
    int line1 = std::max(event->rect().y() / lineSpacing - 2, 0);
    int line2 = std::min(event->rect().bottom() / lineSpacing + 2, m_file.lineCount() - 1);
    // TODO: Consider restricting the painting horizontally too.
    QTextOption textOption;
    textOption.setTabStop(fontMetrics.width(' ') * kTabStopSize);

    for (int line = line1; line <= line2; ++line) {
        int lineStart = m_file.lineStart(line);
        QString lineContent = m_file.lineContent(line).toString();

        // TODO: handle tab stops

        int x = kMarginPx;
        QString oneChar(QChar('\0'));
        for (int column = 0; column < lineContent.size(); ++column) {
            oneChar[0] = lineContent[column];
            CXXSyntaxHighlighter::Kind kind = m_syntaxColoring[lineStart + column];
            Qt::GlobalColor color;
            switch (kind) {
            case CXXSyntaxHighlighter::KindComment:
                color = Qt::darkGreen;
                break;
            case CXXSyntaxHighlighter::KindQuoted:
                color = Qt::darkGreen;
                break;
            case CXXSyntaxHighlighter::KindNumber:
                color = Qt::darkBlue;
                break;
            case CXXSyntaxHighlighter::KindKeyword:
                color = Qt::darkYellow;
                break;
            case CXXSyntaxHighlighter::KindDirective:
                color = Qt::darkBlue;
                break;
            default:
                color = Qt::black;
                break;
            }
            painter.setPen(QColor(color));

            /*
            QRectF bounding(
                        kMarginPx + column * charWidth,
                        kMarginPx + line * lineSpacing,
                        fontMetrics.width(' ') + 1,
                        fontMetrics.height() + 1);
            */
            painter.drawText(
                        x,
                        kMarginPx + lineAscent + line * lineSpacing,
                        oneChar);

            x += fontMetrics.width(oneChar[0]);

            //painter.drawText(bounding, oneChar, textOption);

        }
#if 0
        QRectF bounding(
                    kMarginPx,
                    kMarginPx + line * lineSpacing,
                    fontMetrics.width(' ') * measureLineLength(lineContent.midRef(0), kTabStopSize) + 1,
                    fontMetrics.height() + 1);
        painter.drawText(bounding, lineContent, textOption);
#endif
#if 0
        painter.drawText(
                    kMarginPx,
                         kMarginPx + lineAscent + line * lineSpacing,
                         QString(lineStdString.c_str()));
#endif
    }

    //qDebug() << t.elapsed();

    QWidget::paintEvent(event);
}

QSize SourceWidgetView::sizeHint() const
{
    QFontMetrics fontMetrics(font());
    return QSize(m_maxLineCount * fontMetrics.width(' ') + kMarginPx * 2,
                 m_file.lineCount() * fontMetrics.height() +
                 (m_file.lineCount() - 1) * (fontMetrics.leading() + kExtraLineSpacingPx) +
                 kMarginPx * 2);
}


///////////////////////////////////////////////////////////////////////////////
// SourceWidget

SourceWidget::SourceWidget(Project &project, File &file, QWidget *parent) :
    QScrollArea(parent),
    m_project(project)
{
    setWidget(new SourceWidgetView(project, file));

    setBackgroundRole(QPalette::Base);



#if 0
    setWordWrapMode(QTextOption::NoWrap);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setTextInteractionFlags(Qt::NoTextInteraction);
    setReadOnly(true);
#endif
}

void SourceWidget::setFile(File &file)
{
    if (&this->file() != &file) {
        // The Qt documentation says that we must call show() here if the
        // scroll area is visible, which it presumably is.  However, I don't
        // know whether to call show() before or after setting it as the widget
        // and it's visible even if I don't call show(), so don't bother.
        setWidget(new SourceWidgetView(m_project, file));
    }
}

SourceWidgetView &SourceWidget::sourceWidgetView()
{
    return *qobject_cast<SourceWidgetView*>(widget());
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
