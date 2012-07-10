#ifndef NAV_TABLEWINDOW_H
#define NAV_TABLEWINDOW_H

#include <QMainWindow>
#include <QModelIndex>
#include <QTreeWidgetItem>

class QSortFilterProxyModel;

namespace Nav {

namespace Ui {
class TableWindow;
}

class TableSupplier;

class TableWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit TableWindow(Nav::TableSupplier *supplier, QWidget *parent = 0);
    ~TableWindow();

private slots:
    void selectionChanged();
    void on_treeView_activated(const QModelIndex &index);

private:
    Ui::TableWindow *ui;
    Nav::TableSupplier *supplier;
    QSortFilterProxyModel *proxyModel;
};

} // namespace Nav

#endif // NAV_TABLEWINDOW_H
