#ifndef NAV_TABLEREPORTVIEW_H
#define NAV_TABLEREPORTVIEW_H

#include <QAbstractScrollArea>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QObject>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QSize>
#include <QWidget>
#include <memory>
#include <vector>

#include "Regex.h"

class QHeaderView;
class QStandardItemModel;

namespace Nav {

class TableReport;
class TableReportView_Filterer;
class TableReportView_ProxyReport;
class TableReportView_DirectProxyReport;
class TableReportView_SortProxyReport;
class TableReportView_FilterProxyReport;


///////////////////////////////////////////////////////////////////////////////
// TableReportView_FiltererBase

class TableReportView_FiltererBase : public QObject {
    Q_OBJECT
public:
    TableReportView_FiltererBase(QObject *parent = NULL) :
        QObject(parent)
    {
    }

signals:
    void finished();
};


///////////////////////////////////////////////////////////////////////////////
// TableReportView_Filter

struct TableReportView_Filter {
    std::vector<int> indices;
    std::vector<int> columnWidths;
};


///////////////////////////////////////////////////////////////////////////////
// TableReportView

class TableReportView : public QAbstractScrollArea
{
    Q_OBJECT
public:
    explicit TableReportView(QWidget *parent = 0);
    virtual ~TableReportView();
    void setTableReport(TableReport *report);
    void setFilter(const Regex &filter);
    void setSortOrder(int column, Qt::SortOrder order);
    QSize sizeHint() const;

private:
    void contentChanged();
    int itemCount() const;
    int itemHeight() const;
    QSize estimatedViewportSize();
    int visibleItemCount(const QSize &viewportSize);
    int visibleItemCount();
    void updateScrollBars();
    void positionHeaderView();
    void scrollContentsBy(int dx, int dy);
    bool event(QEvent *event);
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
    void keyPressEvent(QKeyEvent *event);
    int indexFromPoint(const QPoint &pos);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void setSelectedIndex(int index, bool forceReportCallback);
    int selectedReportIndex();
    void ensureIndexVisible(int index);
    TableReportView_ProxyReport &proxyForFilter();

private slots:
    void sortIndicatorChanged();
    void startBackgroundFiltering();
    void finishBackgroundFiltering();

private:
    QHeaderView *m_headerView;
    QWidget *m_headerViewParent;
    QStandardItemModel *m_headerViewModel;
    TableReport *m_report;
    Regex m_filter;
    int m_contentWidth;
    std::unique_ptr<TableReportView_Filterer> m_filterer;
    std::unique_ptr<TableReportView_DirectProxyReport> m_directProxyReport;
    std::unique_ptr<TableReportView_SortProxyReport> m_sortProxyReport;
    std::unique_ptr<TableReportView_FilterProxyReport> m_filterProxyReport;
    int m_selectedIndex;
};

} // namespace Nav

#endif // NAV_TABLEREPORTVIEW_H
