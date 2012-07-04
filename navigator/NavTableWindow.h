#ifndef NAVTABLEWINDOW_H
#define NAVTABLEWINDOW_H

#include <QMainWindow>
#include <QModelIndex>
#include <QTreeWidgetItem>

namespace Ui {
class NavTableWindow;
}

namespace Nav {
class TableSupplier;
}

class NavTableWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit NavTableWindow(Nav::TableSupplier *supplier, QWidget *parent = 0);
    ~NavTableWindow();
    
private slots:
    void on_treeWidget_itemActivated(QTreeWidgetItem *item, int column);

    void on_treeWidget_itemSelectionChanged();

private:
    Ui::NavTableWindow *ui;
    Nav::TableSupplier *supplier;
};

#endif // NAVTABLEWINDOW_H
