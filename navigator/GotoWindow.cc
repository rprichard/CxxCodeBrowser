#include "GotoWindow.h"

#include <QPainter>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QtConcurrentRun>
#include <QtConcurrentMap>
#include <cassert>
#include <re2/re2.h>

#include "MainWindow.h"
#include "Misc.h"
#include "Project.h"
#include "Ref.h"
#include "TextWidthCalculator.h"

using re2::RE2;


namespace Nav {


///////////////////////////////////////////////////////////////////////////////
// Miscellaneous

namespace {

class Regex {
public:
    Regex(const std::string &pattern);
    Regex(const Regex &other);
    bool valid() const                      { return m_re2->ok(); }
    RE2 &re2() const                        { return *m_re2; }
private:
    void initWithPattern(const std::string &pattern);
    std::unique_ptr<RE2> m_re2;
};

} // anonymous namespace

Regex::Regex(const std::string &pattern)
{
    initWithPattern(pattern);
}

Regex::Regex(const Regex &other)
{
    initWithPattern(other.re2().pattern());
}

void Regex::initWithPattern(const std::string &pattern)
{
    bool caseSensitive = false;
    for (unsigned char ch : pattern) {
        if (isupper(ch)) {
            caseSensitive = true;
            break;
        }
    }
    RE2::Options options;
    options.set_case_sensitive(caseSensitive);
    options.set_log_errors(false);
    m_re2 = std::unique_ptr<RE2>(new RE2(pattern, options));
}

static Regex convertFilterIntoRegex(const std::string &filter)
{
#if 0
    QStringList words = filter.split(QRegExp("[^A-Za-z0-9_*]+"), QString::SkipEmptyParts);
    QString regex;

    if (words.size() > 0) {
        for (const QString &word : words) {
            regex += "[^()]*";
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
        regex += "($|[(])";
    }
#endif
    return Regex(filter);
}

// Given a range [0, count), return a vector of (offset, size) tuples that
// cover the range.
static std::vector<std::pair<size_t, size_t> > makeBatches(size_t count)
{
    std::vector<std::pair<size_t, size_t> > result;
    size_t batchSize = std::max<size_t>(100, count / 20);
    size_t batched = 0;
    while (batched < count) {
        size_t batchCount = std::min(batchSize, count - batched);
        result.push_back(std::make_pair(batched, batchCount));
        batched += batchCount;
    }
    return result;
}


///////////////////////////////////////////////////////////////////////////////
// GotoWindowFilter

class GotoWindowFilterer : public GotoWindowFiltererBase {
public:
    typedef int DummyReduceType;

    GotoWindowFilterer(
            const std::vector<Ref> &globalDefs,
            const Regex &pattern,
            TextWidthCalculator &twc) :
        m_futureWatcher(NULL),
        m_globalDefs(globalDefs),
        m_pattern(pattern),
        m_twc(twc)
    {
    }

    ~GotoWindowFilterer()
    {
        if (m_futureWatcher != NULL) {
            m_futureWatcher->cancel();
            m_futureWatcher->waitForFinished();
            delete m_futureWatcher;
        }
    }

    void start()
    {
        assert(m_futureWatcher == NULL);
        m_batches = makeBatches(m_globalDefs.size());
        QFuture<DummyReduceType> future =
                QtConcurrent::mappedReduced
                <DummyReduceType, decltype(m_batches), MapFunc, ReduceFunc>
                (m_batches, MapFunc(*this), ReduceFunc(*this),
                 QtConcurrent::OrderedReduce | QtConcurrent::SequentialReduce);
        m_futureWatcher = new QFutureWatcher<DummyReduceType>();
        connect(m_futureWatcher, SIGNAL(finished()), this, SIGNAL(finished()));
        m_futureWatcher->setFuture(future);
    }

    void wait()
    {
        assert(m_futureWatcher != NULL);
        m_futureWatcher->waitForFinished();
    }

    GotoWindowFilter &result()
    {
        assert(m_futureWatcher != NULL);
        m_futureWatcher->waitForFinished();
        return m_result;
    }

private:
    GotoWindowFilter m_result;
    std::vector<std::pair<size_t, size_t> > m_batches;
    QFutureWatcher<DummyReduceType> *m_futureWatcher;
    const std::vector<Ref> &m_globalDefs;
    Regex m_pattern;
    TextWidthCalculator &m_twc;

    struct MapFunc {
        GotoWindowFilterer &m_parent;
        MapFunc(GotoWindowFilterer &parent) : m_parent(parent) {}
        typedef GotoWindowFilter result_type;

        GotoWindowFilter operator()(const std::pair<size_t, size_t> &range)
        {
            // Make a thread-local copy of the Regex object.  For some reason
            // (locking?), using a single Regex object in all threads is much
            // slower.
            Regex localRegex(m_parent.m_pattern);

            GotoWindowFilter result;
            for (size_t i = range.first, iEnd = range.first + range.second;
                    i < iEnd; ++i) {
                const char *s = m_parent.m_globalDefs[i].symbolCStr();
                if (localRegex.re2().Match(
                            s, 0, strlen(s), RE2::UNANCHORED, NULL, 0)) {
                    result.maxTextWidth = std::max(result.maxTextWidth,
                            m_parent.m_twc.calculate(s));
                    result.indices.push_back(i);
                }
            }
            return result;
        }
    };

    struct ReduceFunc {
        GotoWindowFilterer &m_parent;
        ReduceFunc(GotoWindowFilterer &parent) : m_parent(parent) {}
        typedef DummyReduceType result_type;

        void operator()(DummyReduceType &dummy, const GotoWindowFilter &batch)
        {
            m_parent.m_result.indices.insert(
                        m_parent.m_result.indices.end(),
                        batch.indices.begin(),
                        batch.indices.end());
            m_parent.m_result.maxTextWidth =
                    std::max(m_parent.m_result.maxTextWidth,
                             batch.maxTextWidth);
        }
    };
};


///////////////////////////////////////////////////////////////////////////////
// GotoWindowResults

GotoWindowResults::GotoWindowResults(Project &project, QWidget *parent) :
    QWidget(parent),
    m_project(project),
    m_selectedIndex(-1),
    m_mouseDownIndex(-1)
{
    m_itemMargins = QMargins(3, 1, 3, 1);
}

GotoWindowResults::~GotoWindowResults()
{
}

void GotoWindowResults::setFilter(GotoWindowFilter filter)
{
    m_maxTextWidth = filter.maxTextWidth;

    int originalIndex = m_selectedIndex;
    // Translate the current selection index from the old FilteredSymbols
    // object to the new object.  The QScrollArea containing this widget
    // will probably want to scroll to ensure the selection is still visible.
    int fullSelectedIndex = -1;
    if (m_selectedIndex != -1)
        fullSelectedIndex = m_symbols[m_selectedIndex];
    m_symbols = std::move(filter.indices);
    m_selectedIndex = -1;
    if (fullSelectedIndex != -1) {
        auto range = std::equal_range(m_symbols.begin(), m_symbols.end(),
                                      fullSelectedIndex);
        if (range.first != range.second) {
            m_selectedIndex = range.first - m_symbols.begin();
            assert(static_cast<size_t>(m_selectedIndex) < m_symbols.size());
        }
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
    return m_symbols.size();
}

int GotoWindowResults::itemHeight() const
{
    return effectiveLineSpacing(fontMetrics()) +
            m_itemMargins.top() + m_itemMargins.bottom();
}

QSize GotoWindowResults::sizeHint() const
{
    return QSize(m_maxTextWidth +
                        m_itemMargins.left() + m_itemMargins.right(),
                 itemHeight() * itemCount());
}

void GotoWindowResults::paintEvent(QPaintEvent *event)
{
    const int firstBaseline = fontMetrics().ascent();
    const int itemHeight = this->itemHeight();
    const int item1 = std::max(event->rect().top() / itemHeight - 2, 0);
    const int item2 = std::min(event->rect().bottom() / itemHeight + 2,
                               itemCount() - 1);
    QPainter p(this);
    for (int item = item1; item <= item2; ++item) {
        if (item == m_selectedIndex) {
            // Draw the selection background.
            QStyleOptionViewItemV4 option;
            option.rect = QRect(0, item * itemHeight, width(), itemHeight);
            option.state |= QStyle::State_Selected;
            if (isActiveWindow())
                option.state |= QStyle::State_Active;
            style()->drawPrimitive(QStyle::PE_PanelItemViewRow, &option, &p, this);

            // Use the selection text color.
            p.setPen(palette().color(QPalette::HighlightedText));
        } else {
            // Use the default text color.
            p.setPen(palette().color(QPalette::Text));
        }
        const Ref &ref = m_project.globalSymbolDefinitions()[m_symbols[item]];
        p.drawText(m_itemMargins.left(), firstBaseline + item * itemHeight,
                   ref.symbolCStr());

    }
    QWidget::paintEvent(event);
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
        p.drawText(6, 5 + fontMetrics().ascent(), m_placeholder);
    }
}


///////////////////////////////////////////////////////////////////////////////
// GotoWindow

GotoWindow::GotoWindow(Project &project, QWidget *parent) :
    QWidget(parent),
    m_project(project),
    m_pendingFilterer(NULL)
{
    QFont newFont = font();
    newFont.setPointSize(9);
    newFont.setKerning(true);
    setFont(newFont);

    new QVBoxLayout(this);
    m_editor = new PlaceholderLineEdit("Regex filter (RE2). Case-sensitive if a capital letter exists.");
    layout()->addWidget(m_editor);
    connect(m_editor, SIGNAL(textChanged(QString)), this, SLOT(textChanged()));

    m_scrollArea = new QScrollArea();
    m_results = new GotoWindowResults(m_project);
    layout()->addWidget(m_scrollArea);
    m_scrollArea->setWidget(m_results);
    connect(m_results, SIGNAL(selectionChanged(int)), SLOT(resultsSelectionChanged(int)));
    connect(m_results, SIGNAL(itemClicked(int)), SLOT(navigateToItem(int)));

    // In general, let Qt figure out how to size the GotoWindowResults widget
    // according to its size hint.  When the vertical scroll bar disappears,
    // the GotoWindowResults widget's horizontal size sometimes expands.
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFocusPolicy(Qt::NoFocus);
    m_scrollArea->setBackgroundRole(QPalette::Base);

    setWindowTitle("Go to symbol...");

    // Imitate user input in the search box, but block until the GUI is
    // updated.
    textChanged();
    assert(m_pendingFilterer != NULL);
    m_pendingFilterer->wait();
}

GotoWindow::~GotoWindow()
{
    delete m_pendingFilterer;
}

QSize GotoWindow::sizeHint() const
{
    return QSize(600, 800);
}

void GotoWindow::keyPressEvent(QKeyEvent *event)
{
    const int itemCount = m_results->itemCount();
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

void GotoWindow::resizeResultsWidget()
{
    // As in the SourceWidget, call resize manually because we may need to
    // scroll the window immediately afterwards.
    QSize sizeHint = m_results->sizeHint();
    sizeHint = sizeHint.expandedTo(m_scrollArea->viewport()->size());
    m_results->resize(sizeHint);
}

void GotoWindow::textChanged()
{
    if (m_pendingFilterer != NULL)
        delete m_pendingFilterer;

    Regex pattern = convertFilterIntoRegex(m_editor->text().toStdString());

    {
        QPalette pal;
        if (!pattern.valid())
            pal.setColor(m_editor->foregroundRole(), QColor(Qt::red));
        m_editor->setPalette(pal);
    }

    if (pattern.valid()) {
        m_pendingFilterer = new GotoWindowFilterer(
                    m_project.globalSymbolDefinitions(),
                    pattern,
                    TextWidthCalculator::getCachedTextWidthCalculator(font()));
        connect(m_pendingFilterer, SIGNAL(finished()),
                this, SLOT(symbolFiltererFinished()));
        m_pendingFilterer->start();
    }
}

void GotoWindow::symbolFiltererFinished()
{
    assert(m_pendingFilterer != NULL);
    m_results->setFilter(std::move(m_pendingFilterer->result()));
    delete m_pendingFilterer;
    m_pendingFilterer = NULL;

    resizeResultsWidget();

    // When the selected index becomes -1, this code scrolls to the top.
    int x = m_scrollArea->horizontalScrollBar()->value();
    int y = std::max(0, m_results->selectedIndex() *
                        m_results->itemHeight());
    m_scrollArea->ensureVisible(x, y);

    // Changing the filteredSymbols property can alter the unfiltered index
    // without altering the filtered index, in which case there is no
    // selectionChanged signal.  (The opposite situation can also occur,
    // and then it is necessary to scroll the viewport.)
    navigateToItem(m_results->selectedIndex());
}

void GotoWindow::resultsSelectionChanged(int index)
{
    if (index == -1)
        return;

    // Scroll the selected index into range.
    int x = m_scrollArea->horizontalScrollBar()->value();
    int y = m_results->selectedIndex() * m_results->itemHeight();
    m_scrollArea->ensureVisible(x, y, 0, 0);
    m_scrollArea->ensureVisible(x, y + m_results->itemHeight(), 0, 0);

    navigateToItem(index);
}

void GotoWindow::navigateToItem(int index)
{
    const std::vector<Ref> &globalDefs = m_project.globalSymbolDefinitions();
    if (index >= 0 &&
            index < static_cast<int>(m_results->filteredSymbols().size())) {
        theMainWindow->navigateToRef(
                    globalDefs[m_results->filteredSymbols()[index]]);
    }
}

} // namespace Nav
