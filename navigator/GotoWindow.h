#ifndef NAV_GOTOWINDOW_H
#define NAV_GOTOWINDOW_H

#include <QFontMetricsF>
#include <QFutureWatcher>
#include <QLineEdit>
#include <QPaintEvent>
#include <QScrollArea>
#include <QTableView>
#include <QWidget>
#include <vector>

#include "Ref.h"


namespace re2 {
class RE2;
}

namespace Nav {

class PlaceholderLineEdit;
class Project;
class GotoWindowFilterer;
class TextWidthCalculator;

class GotoWindowFiltererBase : public QObject {
    Q_OBJECT
public:
    GotoWindowFiltererBase(QObject *parent = NULL) : QObject(parent) {}
signals:
    void finished();
};

struct GotoWindowFilter {
    GotoWindowFilter() : maxTextWidth(0) {}
    std::vector<size_t> indices;
    int maxTextWidth;
};


///////////////////////////////////////////////////////////////////////////////
// GotoWindowResults

class GotoWindowResults : public QWidget
{
    Q_OBJECT
public:
    explicit GotoWindowResults(Project &project, QWidget *parent = 0);
    ~GotoWindowResults();
    void setFilter(GotoWindowFilter filter);
    QSize sizeHint() const;
    QSize minimumSizeHint() const { return sizeHint(); }
    const std::vector<size_t> &filteredSymbols() { return m_symbols; }
    int selectedIndex() { return m_selectedIndex; }
    void setSelectedIndex(int index);
    int itemCount() const;
    int itemHeight() const;

signals:
    void selectionChanged(int selectedIndex);
    void itemClicked(int index);

private:
    void paintEvent(QPaintEvent *event);
    int indexAtPoint(QPoint pt);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    Project &m_project;
    QMargins m_itemMargins;
    std::vector<size_t> m_symbols;
    int m_maxTextWidth;
    int m_selectedIndex;
    int m_mouseDownIndex;
};


///////////////////////////////////////////////////////////////////////////////
// GotoWindow

class GotoWindow : public QWidget
{
    Q_OBJECT
public:
    explicit GotoWindow(Project &project, QWidget *parent = 0);
    ~GotoWindow();

private:
    QSize sizeHint() const;
    void keyPressEvent(QKeyEvent *event);
    void resizeResultsWidget();

private slots:
    void textChanged();
    void symbolFiltererFinished();
    void resultsSelectionChanged(int index);
    void navigateToItem(int index);

private:
    Project &m_project;
    PlaceholderLineEdit *m_editor;
    QScrollArea *m_scrollArea;
    GotoWindowResults *m_results;
    GotoWindowFilterer *m_pendingFilterer;
    TextWidthCalculator *m_textWidthCalculator;
};

} // namespace Nav

#endif // NAV_GOTOWINDOW_H
