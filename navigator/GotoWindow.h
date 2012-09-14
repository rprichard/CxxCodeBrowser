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

#include "TextWidthCalculator.h"


namespace re2 {
class RE2;
}

namespace Nav {

class Project;


///////////////////////////////////////////////////////////////////////////////
// FilteredSymbols

class FilteredSymbols : public QObject {
    Q_OBJECT
public:
    FilteredSymbols(
            TextWidthCalculator &textWidthCalculator,
            std::vector<const char*> &symbols,
            const QString &regex);
    virtual ~FilteredSymbols();
    void start();
    void cancel();
    int size() const;
    const char *at(int index) const;
    int findFilteredIndex(int fullIndex) const;
    int filteredIndexToFullIndex(int index) const;
    int maxTextWidth() const;
    void waitForFinished();

signals:
    void done(FilteredSymbols *filteredSymbols);

private:
    struct Batch {
        QFuture<void> future;
        QFutureWatcher<void> watcher;
        std::vector<const char*> *symbols;
        int start;
        int stop;
        int *filtered;
        int filteredCount;
        volatile bool cancelFlag;
        int maxTextWidth;
    };
    void filterBatchThread(QString regex, Batch *batch);

private slots:
    void batchFinished();

private:
    TextWidthCalculator &m_textWidthCalculator;
    std::vector<const char*> &m_symbols;
    std::vector<Batch*> m_batches;
    QString m_regex;
    enum { NotStarted, Started, Done } m_state;
};


///////////////////////////////////////////////////////////////////////////////
// GotoWindowResults

// XXX: If we don't add anything more substantial to the presentation of the
// goto window results, then consider switching to a QListView with a custom
// QAbstractListModel.
// XXX: QListView will probably be too slow.  Consider that it will have to
// measure the size of every string to size the viewport.
class GotoWindowResults : public QWidget
{
    Q_OBJECT
public:
    explicit GotoWindowResults(QWidget *parent = 0);
    ~GotoWindowResults();
    void setFilteredSymbols(FilteredSymbols *symbols);
    QSize sizeHint() const;
    QSize minimumSizeHint() const { return sizeHint(); }
    FilteredSymbols *filteredSymbols() { return m_symbols; }
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
    QMargins m_itemMargins;
    FilteredSymbols *m_symbols;
    int m_selectedIndex;
    int m_mouseDownIndex;
};


///////////////////////////////////////////////////////////////////////////////
// PlaceholderLineEdit

struct PlaceholderLineEdit : QLineEdit
{
    Q_OBJECT
public:
    explicit PlaceholderLineEdit(QString placeholder, QWidget *parent = 0) :
        QLineEdit(parent), m_placeholder(placeholder)
    {
    }
private:
    void paintEvent(QPaintEvent *event);
    QString m_placeholder;
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
    void resizeEvent(QResizeEvent *event);

private slots:
    void textChanged();
    void symbolFilteringDone(FilteredSymbols *filteredSymbols);
    void resultsSelectionChanged(int index);
    void navigateToItem(int index);

private:
    Project &m_project;
    PlaceholderLineEdit *m_editor;
    QScrollArea *m_scrollArea;
    GotoWindowResults *m_results;
    std::vector<int> m_symbolTextWidths;
    std::vector<const char*> m_symbols;
    FilteredSymbols *m_pendingFilteredSymbols;
    TextWidthCalculator *m_textWidthCalculator;
};

} // namespace Nav

#endif // NAV_GOTOWINDOW_H
