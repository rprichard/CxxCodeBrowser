#include "SourceWidget.h"

#include <QDebug>
#include <QTime>
#include <QMenu>
#include <QPainter>
#include <QLabel>
#include <QTextBlock>
#include <QScrollBar>
#include <cassert>

#include "CXXSyntaxHighlighter.h"
#include "File.h"
#include "Misc.h"
#include "Project.h"
#include "ReportRefList.h"
#include "TreeReportWindow.h"
#include "MainWindow.h"

namespace Nav {

const int kTabStopSize = 8;

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

static inline bool isIdentifierChar(QChar ch)
{
    // XXX: What about C++ destructor references, like ~Foo?  Most likely the
    // index should provide a source range rather than a location, and then the
    // GUI should stop trying to pick out identifiers from the text and rely
    // exclusively on the index data.
    unsigned short u = ch.unicode();
    return u <= 127 && (isalnum(u) || u == '_');
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
    m_selection = FileRange();

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

void SourceWidgetView::paintEvent(QPaintEvent *event)
{
    if (m_file != NULL) {
        QFontMetrics fm = fontMetrics();
        const int firstBaseline = fm.ascent() + m_margins.top();
        const int lineSpacing = effectiveLineSpacing(fm);
        QPainter painter(this);

        // Fill in a rectangle for the selected identifier.
        if (!m_selection.isEmpty()) {
            assert(m_selection.start.line == m_selection.end.line);
            QPoint pt1 = locationToPoint(m_selection.start);
            QPoint pt2 = locationToPoint(m_selection.end);
            painter.fillRect(pt1.x(), pt1.y(), (pt2 - pt1).x(), lineSpacing,
                             palette().highlight().color());
        }

        // Paint lines in the clip region.
        //
        // XXX: Consider restricting the painting horizontally too.
        // XXX: Support characters outside the Unicode BMP (i.e. UTF-16
        // surrogate pairs).
        //
        const int line1 = std::max(event->rect().y() / lineSpacing - 2, 0);
        const int line2 = std::min(event->rect().bottom() / lineSpacing + 2,
                                   m_file->lineCount() - 1);
        QColor currentColor(Qt::black);
        painter.setPen(QColor(currentColor));
        const int tabStopPx = fm.width(' ') * kTabStopSize;
        for (int line = line1; line <= line2; ++line) {
            int lineStart = m_file->lineStart(line);
            QString lineContent = m_file->lineContent(line).toString();
            int x = 0;
            QString oneChar(QChar('\0'));
            for (int column = 0; column < lineContent.size(); ++column) {
                oneChar[0] = lineContent[column];
                CXXSyntaxHighlighter::Kind kind =
                        m_syntaxColoring[lineStart + column];
                QColor color(colorForSyntaxKind(kind));


                // TODO: Use index information to color identifiers.  Ideally,
                // we'd color them differently depending upon the kind of
                // identifier.  e.g.:
                //  - types vs
                //  - members vs
                //  - local variables


                // Color the selected characters to match the selected
                // background.
                FileLocation loc(line, column);
                if (loc >= m_selection.start && loc < m_selection.end)
                    color = palette().highlightedText().color();

                if (color != currentColor) {
                    painter.setPen(color);
                    currentColor = color;
                }
                if (oneChar[0] == '\t') {
                    x = (x + tabStopPx) / tabStopPx * tabStopPx;
                } else {
                    painter.drawText(
                                x + m_margins.left(),
                                firstBaseline + line * lineSpacing,
                                oneChar);
                    x += fm.width(oneChar[0]);
                }
            }
        }
    }

    QWidget::paintEvent(event);
}

FileLocation SourceWidgetView::hitTest(QPoint pixel)
{
    if (m_file == NULL)
        return FileLocation();

    QFontMetrics fm = fontMetrics();
    const int tabStopPx = fm.width(' ') * kTabStopSize;
    int hitX = pixel.x() - m_margins.left();
    int hitY = pixel.y() - m_margins.top();
    int line = hitY / effectiveLineSpacing(fm);

    if (line < 0) {
        return FileLocation(0, 0);
    } else if (line >= m_file->lineCount()) {
        return FileLocation(m_file->lineCount(), 0);
    } else {
        QStringRef content = m_file->lineContent(line);
        int x = 0;
        for (int column = 0, lineLength = m_file->lineLength(line);
                column < lineLength; ++column) {
            QChar ch = content.at(column);
            if (ch == '\t') {
                x = (x + tabStopPx) / tabStopPx * tabStopPx;
            } else {
                x += fm.width(ch);
            }
            if (hitX < x)
                return FileLocation(line, column);
        }
        return FileLocation(line, m_file->lineLength(line));
    }
}

QPoint SourceWidgetView::locationToPoint(FileLocation loc)
{
    if (m_file == NULL || loc.isNull())
        return QPoint(m_margins.left(), m_margins.top());

    QFontMetrics fm = fontMetrics();
    const int lineSpacing = effectiveLineSpacing(fm);
    const int tabStopPx = fm.width(' ') * kTabStopSize;
    if (loc.line >= m_file->lineCount()) {
        return QPoint(m_margins.left(),
                      m_margins.top() + lineSpacing * m_file->lineCount());
    }
    QStringRef content = m_file->lineContent(loc.line);
    int x = 0;
    int endColumn = std::min(loc.column, m_file->lineLength(loc.line));
    for (int column = 0; column < endColumn; ++column) {
        QChar ch = content.at(column);
        if (ch == '\t') {
            x = (x + tabStopPx) / tabStopPx * tabStopPx;
        } else {
            x += fm.width(ch);
        }
    }
    return QPoint(m_margins.left() + x,
                  m_margins.top() + lineSpacing * loc.line);
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

FileRange SourceWidgetView::findWordAtLocation(const FileLocation &pt)
{
    if (m_file == NULL || !pt.doesPointAtChar(*m_file))
        return FileRange();
    QStringRef content = m_file->lineContent(pt.line);
    if (!isIdentifierChar(content.at(pt.column)))
        return FileRange(pt, FileLocation(pt.line, pt.column + 1));
    int x1 = pt.column;
    while (x1 - 1 >= 0 && isIdentifierChar(content.at(x1 - 1)))
        x1--;
    int x2 = pt.column;
    while (x2 + 1 < content.size() && isIdentifierChar(content.at(x2 + 1)))
        x2++;
    return FileRange(FileLocation(pt.line, x1), FileLocation(pt.line, x2 + 1));
}

FileRange SourceWidgetView::findWordAtPoint(QPoint pt)
{
    if (m_file != NULL) {
        FileLocation loc = hitTest(pt);
        if (loc.doesPointAtChar(*m_file))
            return findWordAtLocation(loc);
    }
    return FileRange();
}

void SourceWidgetView::mousePressEvent(QMouseEvent *event)
{
    m_selection = findWordAtPoint(event->pos());
    update();
}

void SourceWidgetView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_selection != findWordAtPoint(event->pos())) {
        m_selection = FileRange();
        update();
    }
}

void SourceWidgetView::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_selection.isEmpty()) {
        FileRange identifierClicked;
        if (m_selection == findWordAtPoint(event->pos()))
            identifierClicked = m_selection;
        m_selection = FileRange();
        update();

        // Delay the event handling as long as possible.  Clicking a symbol is
        // likely to cause a jump to another location, which will change the
        // selection (and perhaps the file being displayed).
        if (!identifierClicked.isEmpty()) {
            // TODO: Is this behavior really ideal in the case that one
            // location maps to multiple symbols?
            QStringList symbols = theProject->querySymbolsAtLocation(
                        m_file,
                        identifierClicked.start.line + 1,
                        identifierClicked.start.column + 1);
            if (symbols.size() == 1) {
                Ref ref = m_project.findSingleDefinitionOfSymbol(symbols[0]);
                theMainWindow->navigateToRef(ref);
            }
        }
    }
}

void SourceWidgetView::contextMenuEvent(QContextMenuEvent *event)
{
    if (!m_selection.isEmpty()) {
        if (m_selection != findWordAtPoint(event->pos()))
            m_selection = FileRange();
    }

    if (m_selection.isEmpty()) {
        QMenu *menu = new QMenu();
        bool backEnabled = false;
        bool forwardEnabled = false;
        emit areBackAndForwardEnabled(backEnabled, forwardEnabled);
        menu->addAction(QIcon::fromTheme("go-previous"), "Back",
                        this, SIGNAL(goBack()))->setEnabled(backEnabled);
        menu->addAction(QIcon::fromTheme("go-next"), "Forward",
                        this, SIGNAL(goForward()))->setEnabled(forwardEnabled);
        menu->exec(event->globalPos());
        delete menu;
    } else {
        QMenu *menu = new QMenu();
        int line = m_selection.start.line + 1;
        int column = m_selection.start.column + 1;
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

    update();
}

void SourceWidgetView::actionCrossReferences()
{
    QAction *action = qobject_cast<QAction*>(sender());
    QString symbol = action->data().toString();
    ReportRefList *r = new ReportRefList(theProject, symbol);
    TreeReportWindow *tw = new TreeReportWindow(r);
    tw->show();
}


///////////////////////////////////////////////////////////////////////////////
// SourceWidget

SourceWidget::SourceWidget(Project &project, QWidget *parent) :
    QScrollArea(parent),
    m_project(project)
{
    setWidgetResizable(false);
    setWidget(new SourceWidgetView(QMargins(4, 4, 4, 4), project));
    setBackgroundRole(QPalette::Base);
    setViewportMargins(30, 0, 0, 0);
    m_lineArea = new SourceWidgetLineArea(QMargins(4, 5, 4, 4), this);
    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(layoutSourceWidget()));

    // Configure the widgets to use a small monospace font.  Force characters
    // to have an integral width for simplicity.
    QFont font;
    font.setFamily("Monospace");
    font.setPointSize(8);
    font.setStyleStrategy(QFont::ForceIntegerMetrics);
    widget()->setFont(font);
    m_lineArea->setFont(font);

    connect(&sourceWidgetView(), SIGNAL(goBack()), SIGNAL(goBack()));
    connect(&sourceWidgetView(), SIGNAL(goForward()), SIGNAL(goForward()));
    connect(&sourceWidgetView(),
            SIGNAL(areBackAndForwardEnabled(bool&,bool&)),
            SIGNAL(areBackAndForwardEnabled(bool&,bool&)));

    layoutSourceWidget();
}

void SourceWidget::setFile(File *file)
{
    if (this->file() != file) {
        verticalScrollBar()->setValue(0);
        sourceWidgetView().setFile(file);
        m_lineArea->setLineCount(file != NULL ? file->lineCount() : 0);
        layoutSourceWidget();

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
void SourceWidget::selectIdentifier(int line, int column)
{
    SourceWidgetView &w = sourceWidgetView();
    FileRange r = w.findWordAtLocation(FileLocation(line - 1, column - 1));
    w.setSelection(r);
    QPoint wordTopLeft = w.locationToPoint(r.start);
    ensureVisible(wordTopLeft.x(), wordTopLeft.y());
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

} // namespace Nav
