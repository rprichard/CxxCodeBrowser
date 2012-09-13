#include "GotoWindow.h"

#include <QPainter>
#include <QVBoxLayout>
#include <QtConcurrentRun>
#include <cassert>
#include <re2/re2.h>

#include "MainWindow.h"
#include "Misc.h"
#include "Project.h"

using re2::RE2;


namespace Nav {


///////////////////////////////////////////////////////////////////////////////
// Miscellaneous

static QString convertFilterIntoRegex(const QString &filter)
{
    QStringList words = filter.split(QRegExp("[^A-Za-z0-9_*]+"), QString::SkipEmptyParts);
    QString regex;

    for (QString word : words) {
        regex += ".*";
        regex += "\\b";
        for (QChar ch : word) {
            if (ch.unicode() == '*') {
                regex += "\\w*";
            } else {
                regex += ch;
            }
        }
        regex += "\\b";
    }
    regex += "$";
    return regex;
}


///////////////////////////////////////////////////////////////////////////////
// FilteredSymbols

FilteredSymbols::FilteredSymbols(
        std::vector<const char*> &symbols,
        const QString &regex) :
    m_symbols(symbols), m_regex(regex), m_state(NotStarted)
{
    // Split up the symbols to allow parallelized filtering.  Each batch
    // corresponds to one QtConcurrent::run call, which is run on a Qt-managed
    // thread pool.
    int batchSize = std::max<int>(1000, m_symbols.size() / 20);
    int batched = 0;
    while (batched < static_cast<int>(m_symbols.size())) {
        Batch *batch = new Batch();
        batch->symbols = &m_symbols;
        batch->start = batched;
        batch->stop = std::min<int>(batched + batchSize, m_symbols.size());
        batch->filtered = new int[batch->stop - batch->start];
        batch->filteredCount = 0;
        batch->cancelFlag = false;
        m_batches.push_back(batch);
        batched += batch->stop - batch->start;
    }
}

FilteredSymbols::~FilteredSymbols()
{
    for (Batch *batch : m_batches) {
        // XXX: Is there a concurrency problem here?  The future watcher might
        // be signaling completion just as we're destructing it.  Maybe Qt has
        // the problem under control?
        batch->cancelFlag = true;
        batch->future.waitForFinished();
        delete [] batch->filtered;
        delete batch;
    }
    m_batches.clear();
}

void FilteredSymbols::start()
{
    assert(m_state == NotStarted);
    m_state = Started;
    for (Batch *batch : m_batches) {
        connect(&batch->watcher, SIGNAL(finished()),
                this, SLOT(batchFinished()));
        batch->future = QtConcurrent::run(
                    this, &FilteredSymbols::filterBatchThread, m_regex, batch);
        batch->watcher.setFuture(batch->future);
    }
}

void FilteredSymbols::cancel()
{
    m_state = Done;
    for (Batch *batch : m_batches) {
        batch->cancelFlag = true;
    }
}

void FilteredSymbols::filterBatchThread(QString regex, Batch *batch)
{
    batch->filteredCount = 0;
    RE2::Options options;
    options.set_case_sensitive(true);
    re2::RE2 pattern(regex.toStdString(), options);
    if (!pattern.ok())
        return;
    int *filtered = batch->filtered;
    int filteredCount = 0;
    for (int i = batch->start; i < batch->stop; ++i) {
        if (batch->cancelFlag)
            return;
        const char *s = (*batch->symbols)[i];
        if (pattern.Match(s, 0, strlen(s), RE2::UNANCHORED, NULL, 0))
            filtered[filteredCount++] = i;
    }
    batch->filteredCount = filteredCount;
}

void FilteredSymbols::batchFinished()
{
    if (m_state == Started) {
        for (Batch *batch : m_batches) {
            if (!batch->watcher.isFinished())
                return;
        }
        m_state = Done;
        emit done();
    }
}

int FilteredSymbols::size() const
{
    assert(m_state == Done);
    int result = 0;
    for (const Batch *batch : m_batches)
        result += batch->filteredCount;
    return result;
}

int FilteredSymbols::findFilteredIndex(int fullIndex) const
{
    assert(m_state == Done);
    int filteredIndex = 0;
    for (const Batch *batch : m_batches) {
        for (int i = 0; i < batch->filteredCount; ++i) {
            if (batch->filtered[i] == fullIndex)
                return filteredIndex + i;
        }
        filteredIndex += batch->filteredCount;
    }
    return -1;
}

int FilteredSymbols::filteredIndexToFullIndex(int index) const
{
    assert(m_state == Done);
    for (const Batch *batch : m_batches) {
        if (index < batch->filteredCount)
            return batch->filtered[index];
        index -= batch->filteredCount;
    }
    assert(false);
    return 0;
}

const char *FilteredSymbols::at(int index) const
{
    assert(m_state == Done);
    return m_symbols[filteredIndexToFullIndex(index)];
}


///////////////////////////////////////////////////////////////////////////////
// GotoWindowResults

GotoWindowResults::GotoWindowResults(QWidget *parent) :
    QWidget(parent),
    m_symbols(NULL),
    m_widthHint(1),
    m_selectedIndex(-1),
    m_mouseDownIndex(-1)
{
    m_itemMargins = QMargins(3, 1, 3, 1);
}

GotoWindowResults::~GotoWindowResults()
{
    delete m_symbols;
}

void GotoWindowResults::setFilteredSymbols(FilteredSymbols *symbols)
{
    int originalIndex = m_selectedIndex;
    // Translate the current selection index from the old FilteredSymbols
    // object to the new object.  The QScrollArea containing this widget
    // will probably want to scroll to ensure the selection is still visible.
    int fullSelectedIndex = -1;
    if (m_selectedIndex != -1) {
        assert(m_symbols != NULL);
        fullSelectedIndex =
                m_symbols->filteredIndexToFullIndex(m_selectedIndex);
    }
    delete m_symbols;
    m_symbols = symbols;
    m_widthHint = 0;
    if (m_symbols == NULL) {
        m_selectedIndex = -1;
    } else {
        m_selectedIndex = (fullSelectedIndex != -1)
                ? m_symbols->findFilteredIndex(fullSelectedIndex)
                : -1;
        if (m_selectedIndex == -1 && m_symbols->size() > 0)
            m_selectedIndex = 0;
    }
    updateGeometry();
    update();
    if (m_selectedIndex != originalIndex)
        emit selectionChanged(m_selectedIndex);
}

void GotoWindowResults::setSelectedIndex(int index)
{
    assert(index >= -1 && index < itemCount());
    if (m_selectedIndex != index) {
        m_selectedIndex = index;
        update();
        emit selectionChanged(m_selectedIndex);
    }
}

int GotoWindowResults::itemCount() const
{
    return m_symbols != NULL ? m_symbols->size() : 0;
}

int GotoWindowResults::itemHeight() const
{
    return effectiveLineSpacing(fontMetrics()) +
            m_itemMargins.top() + m_itemMargins.bottom();
}

QSize GotoWindowResults::sizeHint() const
{
    if (m_symbols == NULL)
        return QSize();
    return QSize(m_widthHint, itemHeight() * m_symbols->size());
}

void GotoWindowResults::paintEvent(QPaintEvent *event)
{
    int newWidthHint = m_widthHint;
    if (m_symbols != NULL) {
        const int firstBaseline = fontMetrics().ascent();
        const int itemHeight = this->itemHeight();
        const int item1 = std::max(event->rect().top() / itemHeight - 2, 0);
        const int item2 = std::min(event->rect().bottom() / itemHeight + 2,
                                   m_symbols->size() - 1);
        QPainter p(this);
        for (int item = item1; item <= item2; ++item) {
            if (item == m_selectedIndex) {
                // Draw a selection background.
                p.fillRect(0, item * itemHeight, width(), itemHeight,
                           palette().color(QPalette::Highlight));
                p.setPen(palette().color(QPalette::HighlightedText));
            } else {
                p.setPen(palette().color(QPalette::Text));
            }
            p.drawText(m_itemMargins.left(), firstBaseline + item * itemHeight,
                       m_symbols->at(item));
            newWidthHint = std::max(newWidthHint,
                                    fontMetrics().width(m_symbols->at(item)) +
                                    m_itemMargins.left() +
                                    m_itemMargins.right());
        }
    }
    QWidget::paintEvent(event);

    if (newWidthHint > m_widthHint) {
        m_widthHint = newWidthHint;
        updateGeometry();
    }
}

int GotoWindowResults::indexAtPoint(QPoint pt)
{
    int newIndex = pt.y() / itemHeight();
    if (newIndex >= 0 && newIndex < itemCount())
        return newIndex;
    else
        return -1;
}

void GotoWindowResults::mousePressEvent(QMouseEvent *event)
{
    m_mouseDownIndex = indexAtPoint(event->pos());
    if (m_mouseDownIndex != -1)
        setSelectedIndex(m_mouseDownIndex);
}

void GotoWindowResults::mouseMoveEvent(QMouseEvent *event)
{
    int newIndex = indexAtPoint(event->pos());
    if (newIndex != m_mouseDownIndex)
        m_mouseDownIndex = -1;
    if (newIndex != -1)
        setSelectedIndex(newIndex);
}

void GotoWindowResults::mouseReleaseEvent(QMouseEvent *event)
{
    int newIndex = indexAtPoint(event->pos());
    if (newIndex != m_mouseDownIndex)
        m_mouseDownIndex = -1;
    if (newIndex != -1)
        setSelectedIndex(newIndex);
    if (m_mouseDownIndex != -1)
        emit itemClicked(m_mouseDownIndex);
}


///////////////////////////////////////////////////////////////////////////////
// PlaceholderLineEdit

void PlaceholderLineEdit::paintEvent(QPaintEvent *event)
{
    QLineEdit::paintEvent(event);
    if (text().isEmpty()) {
        QPainter p(this);
        QFont f = font();
        f.setItalic(true);
        p.setFont(f);
        p.setPen(QColor(Qt::lightGray));
        p.drawText(8, 5 + fontMetrics().ascent(), m_placeholder);
    }
}


///////////////////////////////////////////////////////////////////////////////
// GotoWindow

GotoWindow::GotoWindow(Project &project, QWidget *parent) :
    QWidget(parent), m_pendingFilteredSymbols(NULL)
{
    QFont newFont = font();
    newFont.setPointSize(9);
    setFont(newFont);

    new QVBoxLayout(this);
    m_editor = new PlaceholderLineEdit("Space-separated identifier list. The '*' wildcard is accepted.");
    layout()->addWidget(m_editor);
    connect(m_editor, SIGNAL(textChanged(QString)), this, SLOT(textChanged()));

    m_scrollArea = new QScrollArea();
    m_results = new GotoWindowResults;
    layout()->addWidget(m_scrollArea);
    m_scrollArea->setWidget(m_results);
    connect(m_results, SIGNAL(selectionChanged(int)), SLOT(resultsSelectionChanged(int)));
    connect(m_results, SIGNAL(itemClicked(int)), SLOT(navigateToItem(int)));

    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFocusPolicy(Qt::NoFocus);
    m_scrollArea->setBackgroundRole(QPalette::Base);

    setWindowTitle("Go to symbol...");

    struct ConstCharCompare {
        bool operator()(const char *x, const char *y) {
            return strcmp(x, y) < 0;
        }
    };
    project.queryAllSymbols(m_symbols);
    std::sort(m_symbols.begin(), m_symbols.end(), ConstCharCompare());

    textChanged();
}

GotoWindow::~GotoWindow()
{
    delete m_pendingFilteredSymbols;
}

QSize GotoWindow::sizeHint() const
{
    return QSize(600, 800);
}

void GotoWindow::keyPressEvent(QKeyEvent *event)
{
    const int itemCount = m_results->filteredSymbols() != NULL ?
                          m_results->filteredSymbols()->size() : 0;
    int pageSize = std::max(0, m_scrollArea->viewport()->height() /
                            m_results->itemHeight());

    int delta = 0;
    switch (event->key()) {
    case Qt::Key_Up:        delta = -1; break;
    case Qt::Key_Down:      delta = 1; break;
    case Qt::Key_PageUp:    delta = -pageSize; break;
    case Qt::Key_PageDown:  delta = pageSize; break;
    }

    // If the user presses a navigation key (Up,Down,PgUp,PgDn) and the
    // selected index does not change, call navigateToIndex anyway because the
    // sourcewidget may have navigated somewhere else, and there needs to be
    // keyboard-driven way to navigate back.

    if (((event->modifiers() & Qt::ControlModifier) &&
            event->key() == Qt::Key_Q) ||
            event->key() == Qt::Key_Escape ||
            event->key() == Qt::Key_Return) {
        close();
    } else if (delta != 0) {
        if (itemCount > 0) {
            int index = m_results->selectedIndex() + delta;
            index = std::max(0, index);
            index = std::min(itemCount - 1, index);
            m_results->setSelectedIndex(index);
            navigateToItem(m_results->selectedIndex());
        }
    } else if ((event->modifiers() & Qt::ControlModifier) &&
               event->key() == Qt::Key_Home) {
        if (itemCount > 0) {
            m_results->setSelectedIndex(0);
            navigateToItem(m_results->selectedIndex());
        }
    } else if ((event->modifiers() & Qt::ControlModifier) &&
               event->key() == Qt::Key_End) {
        if (itemCount > 0) {
            m_results->setSelectedIndex(itemCount - 1);
            navigateToItem(m_results->selectedIndex());
        }
    } else {
        QWidget::keyPressEvent(event);
    }
}

void GotoWindow::textChanged()
{
    if (m_pendingFilteredSymbols != NULL)
        delete m_pendingFilteredSymbols;
    m_pendingFilteredSymbols =
            new FilteredSymbols(m_symbols,
                                convertFilterIntoRegex(m_editor->text()));
    connect(m_pendingFilteredSymbols,
            SIGNAL(done()),
            SLOT(symbolFilteringDone()));
    m_pendingFilteredSymbols->start();
}

void GotoWindow::symbolFilteringDone()
{
    if (m_pendingFilteredSymbols != NULL) {
        m_results->setFilteredSymbols(m_pendingFilteredSymbols);
        m_pendingFilteredSymbols = NULL;

        // Here I'm going to try setting QScrollArea's widgetResizable property
        // to true and *also* calling resize early so I can scroll to the
        // bottom of the widget.  I'm doing this because the results window
        // expands its width in its paint event handler, and Qt will then
        // automatically resize the results window if widgetResizable is true.
        QSize sizeHint = m_results->sizeHint();
        sizeHint = sizeHint.expandedTo(m_scrollArea->viewport()->size());
        m_results->resize(sizeHint);

        int y = std::max(0, m_results->selectedIndex() *
                            m_results->itemHeight());
        m_scrollArea->ensureVisible(0, y);

        // Changing the filteredSymbols property can alter the unfiltered index
        // without altering the filtered index, in which case there is no
        // selectionChanged signal.  (The opposite situation can also occur,
        // and then it is necessary to scroll the viewport.)
        navigateToItem(m_results->selectedIndex());
    }
}

void GotoWindow::resultsSelectionChanged(int index)
{
    if (index == -1)
        return;

    // Scroll the selected index into range.
    int y = m_results->selectedIndex() * m_results->itemHeight();
    m_scrollArea->ensureVisible(0, y, 0, 0);
    m_scrollArea->ensureVisible(0, y + m_results->itemHeight(), 0, 0);

    navigateToItem(index);
}

void GotoWindow::navigateToItem(int index)
{
    if (index == -1)
        return;

    // Lookup the (first?) definition of the symbol and navigate to it.
    // TODO: What about multiple definitions of the same symbol?
    theMainWindow->navigateToSomeDefinitionOfSymbol(
                m_results->filteredSymbols()->at(index));
}

} // namespace Nav
