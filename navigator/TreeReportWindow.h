#ifndef NAV_TREEREPORTWINDOW_H
#define NAV_TREEREPORTWINDOW_H

#include <QMainWindow>
#include <QModelIndex>
#include <QTreeWidgetItem>
#include "TreeReport.h"

namespace Nav {

namespace Ui {
class TreeReportWindow;
}

class TreeReportWindow : public QMainWindow
{
    Q_OBJECT

private:
    struct TreeWidgetItem : QTreeWidgetItem {
        TreeReport::Index treeReportIndex;
    };

public:
    explicit TreeReportWindow(TreeReport *treeReport, QWidget *parent = 0);

private:
    QList<QTreeWidgetItem*> createChildTreeWidgetItems(
            const TreeReport::Index &parent);
    QTreeWidgetItem *createTreeWidgetItem(
            const TreeReport::Index &index);

public:
    ~TreeReportWindow();

private slots:
    void on_treeWidget_itemSelectionChanged();
    void on_treeWidget_itemActivated(QTreeWidgetItem *item, int column);

private:
    Ui::TreeReportWindow *ui;
    TreeReport *treeReport;
};

} // namespace Nav

#endif // NAV_TREEREPORTWINDOW_H
