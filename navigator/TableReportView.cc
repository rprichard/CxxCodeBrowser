#include "TableReportView.h"

#include <QFuture>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>
#include <QStandardItemModel>
#include <QtConcurrentMap>
#include <cassert>
#include <memory>
#include <vector>

#include "Misc.h"
#include "Regex.h"
#include "TableReport.h"
#include "TextWidthCalculator.h"

namespace Nav {


///////////////////////////////////////////////////////////////////////////////
// Miscellaneous

// Given a range [0, count), return a vector of (offset, size) tuples that
// cover the range.
static std::vector<std::pair<int, int> > makeBatches(int count)
{
    std::vector<std::pair<int, int> > result;
    int batchSize = std::max(100, count / 20);
    int batched = 0;
    while (batched < count) {
        int batchCount = std::min(batchSize, count - batched);
        result.push_back(std::make_pair(batched, batchCount));
        batched += batchCount;
    }
    return result;
}


///////////////////////////////////////////////////////////////////////////////
// TableReportView_ProxyReport

struct TableReportView_ProxyReport {
    virtual ~TableReportView_ProxyReport() {}
    virtual int rowCount()              { return tableReport().rowCount(); }
    virtual int mapToSource(int row)    { return row; }
    virtual int mapFromSource(int row)  { return row; }
    virtual TableReportView_ProxyReport *nextProxy()    { return NULL; }

    virtual int mapToTableReport(int row) {
        row = mapToSource(row);
        assert(row >= 0);
        TableReportView_ProxyReport *next = nextProxy();
        if (next != NULL)
            row = next->mapToTableReport(row);
        assert(row >= 0);
        return row;
    }

    virtual int mapFromTableReport(int row) {
        TableReportView_ProxyReport *next = nextProxy();
        if (next != NULL)
            row = next->mapFromTableReport(row);
        if (row >= 0)
            row = mapFromSource(row);
        return row;
    }

    virtual TableReport &tableReport() {
        TableReportView_ProxyReport *next = nextProxy();
        assert(next != NULL);
        return next->tableReport();
    }
};


///////////////////////////////////////////////////////////////////////////////
// TableReportView_DirectProxyReport

struct TableReportView_DirectProxyReport : TableReportView_ProxyReport {
    TableReportView_DirectProxyReport(
            TableReport &report) :
        m_report(report)
    {
    }

    virtual int mapToTableReport(int row) { return row; }
    virtual int mapFromTableReport(int row) { return row; }
    virtual TableReport &tableReport() { return m_report; }
private:
    TableReport &m_report;
};


///////////////////////////////////////////////////////////////////////////////
// TableReportView_SortProxyReport

struct TableReportView_SortProxyReport : TableReportView_ProxyReport {
    TableReportView_SortProxyReport(
            TableReportView_ProxyReport &report,
            int sortColumn,
            Qt::SortOrder sortOrder) :
        m_report(report)
    {
        int rowCount = report.rowCount();
        m_remap.resize(rowCount);
        for (int i = 0; i < rowCount; ++i)
            m_remap[i] = i;
        TableReport &tableReport = this->tableReport();
        bool isDescending = (sortOrder == Qt::DescendingOrder);
        // isDirect is a performance optimization -- it avoids two
        // mapToTableReport virtual function calls per comparison.
        bool isDirect = report.nextProxy() == NULL;
        std::sort(m_remap.begin(), m_remap.end(),
                  [&report, &tableReport, isDescending, isDirect, sortColumn]
                        (int row1, int row2) -> bool {
            bool result;
            int row1mapped = isDirect ? row1 : report.mapToTableReport(row1);
            int row2mapped = isDirect ? row2 : report.mapToTableReport(row2);
            int compare = tableReport.compare(
                        row1mapped, row2mapped, sortColumn);
            if (compare < 0)
                result = true;
            else if (compare > 0)
                result = false;
            else
                result = row1 < row2;
            return isDescending ? !result : result;
        });
    }

    virtual TableReportView_ProxyReport *nextProxy() { return &m_report; }

    virtual int mapToSource(int row)
    {
        assert(row >= 0);
        return m_remap[row];
    }

    virtual int mapFromSource(int row)
    {
        for (int i = 0; i < static_cast<int>(m_remap.size()); ++i) {
            if (m_remap[i] == row)
                return i;
        }
        return -1;
    }

private:
    TableReportView_ProxyReport &m_report;
    std::vector<int> m_remap;
};


///////////////////////////////////////////////////////////////////////////////
// TableReportView_FilterProxyReport

struct TableReportView_FilterProxyReport : TableReportView_ProxyReport {
    TableReportView_FilterProxyReport(TableReportView_ProxyReport &report) :
        m_report(report)
    {
    }

    // The indices in the remapped table must be in strictly ascending order.
    void setFilter(std::vector<int> &&remap) { m_remap = std::move(remap); }

    virtual int rowCount() { return m_remap.size(); }
    virtual TableReportView_ProxyReport *nextProxy() { return &m_report; }
    virtual int mapToSource(int row) { assert(row >= 0); return m_remap[row]; }

    virtual int mapFromSource(int row) {
        auto range = std::equal_range(m_remap.begin(), m_remap.end(), row);
        if (range.first == range.second)
            return -1;
        return range.first - m_remap.begin();
    }

private:
    TableReportView_ProxyReport &m_report;
    std::vector<int> m_remap;
};


///////////////////////////////////////////////////////////////////////////////
// TableReportView_Filterer

// This extra width accounts for:
//  - padding between the left edge of the column and the text
//  - the sort indicator
const int kColumnHeaderExtraWidth = 28;

// Padding surrounding a single (row, col) table item.
const QMargins kTableItemMargins = QMargins(6, 1, 10, 1);

class TableReportView_Filterer : public TableReportView_FiltererBase {
public:
    typedef int DummyReduceType;

    TableReportView_Filterer(
            TableReportView_ProxyReport &report,
            const Regex &pattern,
            TextWidthCalculator &twc) :
        m_report(report),
        m_pattern(pattern),
        m_twc(twc)
    {
        // Initialize the results struct with the widths of the column
        // headings.
        QStringList columns = m_report.tableReport().columns();
        m_result.columnWidths.resize(columns.size());
        for (int col = 0; col < columns.size(); ++col) {
            m_result.columnWidths[col] = m_twc.calculate(columns[col]) +
                    kColumnHeaderExtraWidth;
        }
    }

    ~TableReportView_Filterer()
    {
        if (m_filterFutureWatcher) {
            m_filterFutureWatcher->cancel();
            m_filterFutureWatcher->waitForFinished();
            m_filterFutureWatcher.reset();
        }
    }

    void start()
    {
        assert(!m_filterFutureWatcher);
        m_batches = makeBatches(m_report.rowCount());
        QFuture<DummyReduceType> future =
                QtConcurrent::mappedReduced
                <DummyReduceType, decltype(m_batches), MapFunc, ReduceFunc>
                (m_batches, MapFunc(*this), ReduceFunc(*this),
                 QtConcurrent::OrderedReduce | QtConcurrent::SequentialReduce);
        m_filterFutureWatcher =
                std::unique_ptr<QFutureWatcher<DummyReduceType> >(
                    new QFutureWatcher<DummyReduceType>());
        connect(m_filterFutureWatcher.get(), SIGNAL(finished()),
                this, SIGNAL(finished()));
        m_filterFutureWatcher->setFuture(future);
    }

    void wait()
    {
        assert(m_filterFutureWatcher);
        m_filterFutureWatcher->waitForFinished();
    }

    TableReportView_Filter &result()
    {
        assert(m_filterFutureWatcher);
        m_filterFutureWatcher->waitForFinished();
        return m_result;
    }

private:
    TableReportView_Filter m_result;
    std::vector<std::pair<int, int> > m_batches;
    std::unique_ptr<QFutureWatcher<DummyReduceType> > m_filterFutureWatcher;
    TableReportView_ProxyReport &m_report;
    Regex m_pattern;
    int m_sortColumn;
    Qt::SortOrder m_sortOrder;
    TextWidthCalculator &m_twc;

    struct MapFunc {
        TableReportView_Filterer &m_parent;
        MapFunc(TableReportView_Filterer &parent) :
            m_parent(parent)
        {
        }

        typedef TableReportView_Filter result_type;

        TableReportView_Filter operator()(
                const std::pair<int, int> &range)
        {
            TableReportView_Filter result;

            // Make a thread-local copy of the Regex object.  For some reason
            // (locking?), using a single Regex object in all threads is much
            // slower.
            Regex localRegex(m_parent.m_pattern);

            TableReportView_ProxyReport &proxy = m_parent.m_report;
            TableReport &report = proxy.tableReport();
            TextWidthCalculator &twc = m_parent.m_twc;
            const int columnCount = report.columns().count();
            result.columnWidths.resize(columnCount);

            std::string tempBuf;
            for (int i = range.first, iEnd = range.first + range.second;
                    i < iEnd; ++i) {
                int mappedRow = proxy.mapToTableReport(i);
                if (report.filter(mappedRow, localRegex, tempBuf)) {
                    result.indices.push_back(i);
                    for (int col = 0; col < columnCount; ++col) {
                        const char *itemText =
                                report.text(mappedRow, col, tempBuf);
                        result.columnWidths[col] = std::max(
                                    result.columnWidths[col],
                                    twc.calculate(itemText) +
                                    kTableItemMargins.left() +
                                    kTableItemMargins.right());
                    }
                }
            }
            return result;
        }
    };

    struct ReduceFunc {
        TableReportView_Filterer &m_parent;
        ReduceFunc(TableReportView_Filterer &parent) :
            m_parent(parent)
        {
        }

        typedef DummyReduceType result_type;

        void operator()(
                DummyReduceType &dummy,
                const TableReportView_Filter &batch)
        {
            m_parent.m_result.indices.insert(
                        m_parent.m_result.indices.end(),
                        batch.indices.begin(),
                        batch.indices.end());
            const int columnCount =
                    m_parent.m_report.tableReport().columns().size();
            for (int col = 0; col < columnCount; ++col) {
                m_parent.m_result.columnWidths[col] = std::max(
                            m_parent.m_result.columnWidths[col],
                            batch.columnWidths[col]);
            }
        }
    };
};


///////////////////////////////////////////////////////////////////////////////
// TableReportView

// TODO: Replace this with QStyle hint calls.
const int kViewportScrollbarMargin = 3;

TableReportView::TableReportView(QWidget *parent) :
    QAbstractScrollArea(parent),
    m_report(NULL),
    m_filter(""),
    m_contentWidth(0),
    m_selectedIndex(-1)
{
    // TODO: This probably belongs somewhere else, along with all the other
    // setFont calls.
    QFont newFont = font();
    newFont.setPointSize(9);
    setFont(newFont);

    m_headerViewModel = new QStandardItemModel(this);
    m_headerViewParent = new QWidget(this);
    m_headerView = new QHeaderView(Qt::Horizontal, m_headerViewParent);
    m_headerView->setModel(m_headerViewModel);
    m_headerView->setStretchLastSection(true);
    m_headerView->setClickable(true);
    m_headerView->setSortIndicatorShown(true);
    m_headerView->setResizeMode(QHeaderView::Fixed);
    m_headerView->setDefaultAlignment(Qt::AlignLeft);

    connect(m_headerView, SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)),
            this, SLOT(sortIndicatorChanged()));
}

TableReportView::~TableReportView()
{
}

template <typename T>
std::unique_ptr<T> make_unique_ptr(T *p) {
    return std::unique_ptr<T>(p);
}

// The TableReportView class does not take ownership of the TableReport.
void TableReportView::setTableReport(TableReport *report)
{
    if (report == m_report)
        return;

    // Reset everything.
    m_filterProxyReport.reset();
    m_sortProxyReport.reset();
    m_directProxyReport.reset();
    m_report = NULL;
    m_filterer.reset();
    m_headerViewModel->setHorizontalHeaderLabels(QStringList());
    m_headerView->setSortIndicator(-1, Qt::AscendingOrder);

    if (report == NULL)
        return;

    m_report = report;
    m_directProxyReport = make_unique_ptr(
                new TableReportView_DirectProxyReport(*report));
    QStringList columns = m_report->columns();
    m_headerViewModel->setHorizontalHeaderLabels(columns);
    for (int col = 0; col < columns.size(); ++col)
        m_headerView->resizeSection(col, 100);
    m_contentWidth = columns.size() * 100;
    startBackgroundFiltering();
    finishBackgroundFiltering();
}

void TableReportView::setFilter(const Regex &filter)
{
    if (filter == m_filter)
        return;
    assert(filter.valid());
    m_filter = filter;
    startBackgroundFiltering();

    if (m_selectedIndex == -1)
        ensureIndexVisible(0);
}

void TableReportView::setSortOrder(int column, Qt::SortOrder order)
{
    if (column == m_headerView->sortIndicatorSection() &&
            order == m_headerView->sortIndicatorOrder())
        return;
    m_headerView->setSortIndicator(column, order);
}

QSize TableReportView::sizeHint() const
{
    return QSize(m_contentWidth + frameWidth() * 2 +
                 verticalScrollBar()->sizeHint().width() +
                 kViewportScrollbarMargin,
                 itemCount() * itemHeight() + frameWidth() * 2 +
                 m_headerView->sizeHint().height() +
                 horizontalScrollBar()->sizeHint().height() +
                 kViewportScrollbarMargin);
}

void TableReportView::contentChanged()
{
    updateScrollBars();
    positionHeaderView();
    viewport()->update();
}

int TableReportView::itemCount() const
{
    return m_filterProxyReport ? m_filterProxyReport->rowCount() : 0;
}

int TableReportView::itemHeight() const
{
    return effectiveLineSpacing(fontMetrics()) +
            kTableItemMargins.top() + kTableItemMargins.bottom();
}

// It's better if this size is an understimate rather than an overestimate.
// It takes the current scrollbar status into account, unlike
// viewport()->size(), which uses layout information cached somewhere and
// only incorporates scrollbar status after some layout process completes.
QSize TableReportView::estimatedViewportSize()
{
    bool vbar = false;
    bool hbar = false;
    int viewportHeightEstimate =
            height() - m_headerView->sizeHint().height() -
            frameWidth() * 2;
    int viewportWidthEstimate = width() - frameWidth() * 2;
    const int barW = verticalScrollBar()->sizeHint().width() +
            kViewportScrollbarMargin;
    const int barH = horizontalScrollBar()->sizeHint().height() +
            kViewportScrollbarMargin;

    const int itemH = itemHeight();
    const int itemC = itemCount();

    for (int round = 0; round < 2; ++round) {
        if (!vbar && viewportHeightEstimate / itemH < itemC) {
            vbar = true;
            viewportWidthEstimate -= barW;
        }
        if (!hbar && viewportWidthEstimate < m_contentWidth) {
            hbar = true;
            viewportHeightEstimate -= barH;
        }
    }

    return QSize(viewportWidthEstimate, viewportHeightEstimate);
}

int TableReportView::visibleItemCount(const QSize &viewportSize)
{
    return std::max(1, viewportSize.height() / itemHeight());
}

int TableReportView::visibleItemCount()
{
    return visibleItemCount(estimatedViewportSize());
}

void TableReportView::updateScrollBars()
{
    if (!m_filterProxyReport) {
        horizontalScrollBar()->setRange(0, 0);
        verticalScrollBar()->setRange(0, 0);
        return;
    }

    QSize viewportSize = estimatedViewportSize();
    const int visibleItemCount = this->visibleItemCount(viewportSize);
    verticalScrollBar()->setRange(0, itemCount() - visibleItemCount);
    verticalScrollBar()->setPageStep(visibleItemCount);

    horizontalScrollBar()->setRange(0, m_contentWidth - viewportSize.width());
    horizontalScrollBar()->setPageStep(viewportSize.width());
    horizontalScrollBar()->setSingleStep(
                fontMetrics().averageCharWidth() * 20);
}

void TableReportView::positionHeaderView()
{
    const int headerHeight = m_headerView->sizeHint().height();
    const int headerWidth = std::max(
                m_contentWidth,
                viewport()->width());
    m_headerViewParent->setGeometry(
                frameWidth(), frameWidth(),
                viewport()->width(), headerHeight);
    m_headerView->setGeometry(-horizontalScrollBar()->value(),
                              0,
                              headerWidth,
                              headerHeight);
}

void TableReportView::scrollContentsBy(int dx, int dy)
{
    positionHeaderView();
    viewport()->scroll(dx, dy * itemHeight());
}

bool TableReportView::event(QEvent *event)
{
    // Resizing in a QAbstractScrollArea is non-obvious.  Earlier, I had the
    // setViewportMargins call in the positionHeaderView method.  When the
    // TableReportView was resized, the typical flow of calls was:
    //    resizeEvent()
    //       updateScrollBars()
    //       positionHeaderView()
    //          setViewportMargins()
    //          setGeometry()  (on m_headerViewParent)
    //          setGeometry()  (on m_headerView)
    // It failed because setViewportMargins() called resizeEvent(),
    // reentrantly.  The earlier updateScrollBars() call would sometimes toggle
    // the scroll bars on or off, and then the reentrant resizeEvent() would
    // finally update the viewport size to take the new scrollbar state into
    // account.  By a fluke, positionHeaderView() read viewport()->width()
    // before the setViewportMargins() call and did not reread it afterwards,
    // so it used the stale viewport size.  Because Qt had already called
    // resizeEvent() with the new viewport size, it apparently assumed it did
    // not need to call resizeEvent() again later.
    //
    // I'm trying to avoid this problem using this strategy:
    // (1) Catch the resize event for the TableReportView widget itself rather
    //     than the viewport.  It seems broken that the viewport's resize event
    //     would itself resize the viewport.  I assume that the
    //     QAbstractScrollArea::event method will do the actual viewport and
    //     scrollbar layout, and when it does, the scrollbars will have already
    //     been hidden or shown.
    // (2) Call setViewportMargins first.  Note that we can't call it in the
    //     constructor because the size hint is still 0 at that time.
    // (3) When the TableReportView is resized, we can set the viewport margin
    //     and scrollbar ranges/visibility authoritatively.  We can't set the
    //     header view position accurately because the viewport size could
    //     change.  But just in case it doesn't, set a tentative header
    //     position.  We still have a resizeEvent handler that will receive the
    //     updated viewport size.

    if (event->type() == QEvent::Resize) {
        setViewportMargins(0, m_headerView->sizeHint().height(), 0, 0);
        updateScrollBars();
        positionHeaderView();
    }
    return QAbstractScrollArea::event(event);
}

void TableReportView::resizeEvent(QResizeEvent *event)
{
    positionHeaderView();
}

void TableReportView::paintEvent(QPaintEvent *event)
{
    if (!m_filterProxyReport)
        return;

    const int viewRow1 = verticalScrollBar()->value();
    const int drawRow1 = viewRow1 + event->rect().top() / itemHeight();
    const int drawRow2 = std::min(
                itemCount() - 1,
                drawRow1 + visibleItemCount());
    if (drawRow1 >= itemCount())
        return;
    assert(drawRow2 >= drawRow1);

    QPainter painter(viewport());
    const int lineSpacing = itemHeight();
    const int ascent = fontMetrics().ascent() + kTableItemMargins.top();
    int y = (drawRow1 - viewRow1) * lineSpacing;

    const int columnCount = m_report->columns().count();
    std::string tempBuf;
    for (int row = drawRow1; row <= drawRow2; ++row) {

        if (row == m_selectedIndex) {
            // Draw the selection background.
            QStyleOptionViewItemV4 option;
            option.rect = QRect(0, y, viewport()->width(), lineSpacing);
            option.state |= QStyle::State_Selected;
            if (isActiveWindow())
                option.state |= QStyle::State_Active;
            style()->drawPrimitive(QStyle::PE_PanelItemViewRow,
                                   &option, &painter, this);

            // Use the selection text color.
            painter.setPen(palette().color(QPalette::HighlightedText));
        } else {
            // Use the default text color.
            painter.setPen(palette().color(QPalette::Text));
        }

        for (int col = 0; col < columnCount; ++col) {
            int x = m_headerView->sectionViewportPosition(col);
            x -= horizontalScrollBar()->value();
            x += kTableItemMargins.left();
            const char *text = m_report->text(
                        m_filterProxyReport->mapToTableReport(row),
                        col, tempBuf);
            painter.drawText(x, y + ascent, text);
        }

        y += lineSpacing;
    }
}

void TableReportView::keyPressEvent(QKeyEvent *event)
{
    int delta = 0;
    switch (event->key()) {
    case Qt::Key_Up:        delta = -1; break;
    case Qt::Key_Down:      delta = 1; break;
    case Qt::Key_PageUp:    delta = -visibleItemCount(); break;
    case Qt::Key_PageDown:  delta = visibleItemCount(); break;
    }

    // Call setSelectedIndex with forceReportCallback=true to trigger a
    // TableReport::select callback even if the selected index did not change.
    // For definition/path/ref report windows, we'd like pressing navigation
    // keys (Up,Down,PgUp,PgDn) to always navigate the SourceWidget to the
    // selected item, even if it was already selected.

    if (delta != 0) {
        if (itemCount() > 0) {
            int index = m_selectedIndex + delta;
            index = std::max(0, index);
            index = std::min(itemCount() - 1, index);
            setSelectedIndex(index, /*forceReportCallback=*/true);
        }
    } else if (event->key() == Qt::Key_Home) {
        if (itemCount() > 0) {
            setSelectedIndex(0, /*forceReportCallback=*/true);
        }
    } else if (event->key() == Qt::Key_End) {
        if (itemCount() > 0) {
            setSelectedIndex(itemCount() - 1, /*forceReportCallback=*/true);
        }
    } else if (event->key() == Qt::Key_Enter ||
               event->key() == Qt::Key_Return) {
        const int selectedReportIndex = this->selectedReportIndex();
        if (selectedReportIndex != -1)
            m_report->activate(selectedReportIndex);
    } else {
        QAbstractScrollArea::keyPressEvent(event);
    }
}

int TableReportView::indexFromPoint(const QPoint &pos)
{
    int index = pos.y() / itemHeight() + verticalScrollBar()->value();
    index = std::max(0, index);
    index = std::min(itemCount() - 1, index);
    return index;
}

void TableReportView::mousePressEvent(QMouseEvent *event)
{
    int index = indexFromPoint(event->pos());
    if (index == -1)
        return;
    setSelectedIndex(index, /*forceReportCallback=*/true);
}

void TableReportView::mouseMoveEvent(QMouseEvent *event)
{
    int index = indexFromPoint(event->pos());
    if (index == -1)
        return;
    setSelectedIndex(index, /*forceReportCallback=*/false);
}

void TableReportView::mouseDoubleClickEvent(QMouseEvent *event)
{
    int index = indexFromPoint(event->pos());
    if (index == -1)
        return;
    if (index != m_selectedIndex) {
        setSelectedIndex(index, /*forceReportCallback=*/true);
    } else {
        int reportIndex = m_filterProxyReport->mapToTableReport(index);
        assert(reportIndex != -1);
        m_report->activate(reportIndex);
    }
}

void TableReportView::setSelectedIndex(int index, bool forceReportCallback)
{
    bool changed = false;
    if (m_selectedIndex != index) {
        m_selectedIndex = index;
        ensureIndexVisible(index);
        viewport()->update();
        changed = true;
    }

    if (changed || forceReportCallback) {
        const int reportIndex = selectedReportIndex();
        if (reportIndex != -1)
            m_report->select(reportIndex);
    }
}

int TableReportView::selectedReportIndex()
{
    if (m_selectedIndex == -1 || !m_filterProxyReport)
        return -1;
    return m_filterProxyReport->mapToTableReport(m_selectedIndex);
}

void TableReportView::ensureIndexVisible(int index)
{
    index = std::min(itemCount() - 1, index);
    if (index == -1) {
        verticalScrollBar()->setValue(0);
    } else {
        const int page = visibleItemCount();
        if (index < verticalScrollBar()->value()) {
            verticalScrollBar()->setValue(index);
        } else if (index >= verticalScrollBar()->value() + page) {
            verticalScrollBar()->setValue(index - page + 1);
        }
    }
}

TableReportView_ProxyReport &TableReportView::proxyForFilter()
{
    if (m_sortProxyReport) {
        return *m_sortProxyReport;
    } else {
        assert(m_directProxyReport);
        return *m_directProxyReport;
    }
}

void TableReportView::sortIndicatorChanged()
{
    int oldSelection = -1;
    if (m_selectedIndex != -1 && m_filterProxyReport)
        oldSelection = m_filterProxyReport->mapToTableReport(m_selectedIndex);
    m_selectedIndex = -1;

    m_filterer.reset();
    m_filterProxyReport.reset();
    m_sortProxyReport.reset();
    if (m_directProxyReport && m_headerView->sortIndicatorSection() >= 0) {
        m_sortProxyReport = make_unique_ptr(
                    new TableReportView_SortProxyReport(
                        *m_directProxyReport,
                        m_headerView->sortIndicatorSection(),
                        m_headerView->sortIndicatorOrder()));
    }
    startBackgroundFiltering();
    finishBackgroundFiltering();
    contentChanged();

    if (oldSelection != -1 && m_filterProxyReport) {
        setSelectedIndex(
                    m_filterProxyReport->mapFromTableReport(oldSelection),
                    /*forceReportCallback=*/false);
    }
}

void TableReportView::startBackgroundFiltering()
{
    if (!m_directProxyReport)
        return;

    TextWidthCalculator &twc =
            TextWidthCalculator::getCachedTextWidthCalculator(font());
    m_filterer = make_unique_ptr(
                new TableReportView_Filterer(proxyForFilter(), m_filter, twc));
    connect(m_filterer.get(), SIGNAL(finished()),
            this, SLOT(finishBackgroundFiltering()));
    m_filterer->start();
}

void TableReportView::finishBackgroundFiltering()
{
    if (!m_filterer)
        return;

    // Translate the selection index to the underlying data source.
    int oldSelection = -1;
    if (m_selectedIndex != -1 && m_filterProxyReport)
        oldSelection = m_filterProxyReport->mapToSource(m_selectedIndex);
    m_selectedIndex = -1;

    TableReportView_Filter &result = m_filterer->result();
    m_filterProxyReport = make_unique_ptr(
                new TableReportView_FilterProxyReport(
                    proxyForFilter()));
    m_filterProxyReport->setFilter(std::move(result.indices));
    const int columnCount = m_report->columns().size();
    m_contentWidth = 0;
    for (int col = 0; col < columnCount; ++col) {
        m_headerView->resizeSection(col, result.columnWidths[col]);
        m_contentWidth += result.columnWidths[col];
    }
    m_filterer.reset();

    contentChanged();

    // Search for the selected item in the new filter and select it.
    if (oldSelection != -1) {
        setSelectedIndex(m_filterProxyReport->mapFromSource(oldSelection),
                         /*forceReportCallback=*/false);
    }
}

} // namespace Nav
