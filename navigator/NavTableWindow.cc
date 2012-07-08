#include "NavTableWindow.h"
#include "ui_NavTableWindow.h"
#include "TableSupplier.h"
#include <QDebug>
#include <QKeySequence>
#include <QShortcut>
#include <QStandardItemModel>
#include <QStringList>
#include <QTreeWidgetItem>
#include <cstdlib>

NavTableWindow::NavTableWindow(Nav::TableSupplier *supplier, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::NavTableWindow),
    supplier(supplier)
{
    ui->setupUi(this);

    setWindowTitle(supplier->getTitle());

    // Register Ctrl+Q.
    QShortcut *shortcut = new QShortcut(QKeySequence("Ctrl+Q"), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(close()));

    QStringList columnLabels = supplier->getColumnLabels();
    ui->treeWidget->setHeaderLabels(columnLabels);
    QList<QList<QVariant> > data = supplier->getData();
    foreach (const QList<QVariant> &row, data) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setData(0, Qt::UserRole, row[0]);
        for (int col = 1; col < row.size(); ++col) {
            item->setData(col - 1, Qt::DisplayRole, row[col]);
        }
        ui->treeWidget->addTopLevelItem(item);
    }

    for (int i = 0; i < columnLabels.size(); ++i) {
        ui->treeWidget->resizeColumnToContents(i);
        ui->treeWidget->setColumnWidth(
                    i, ui->treeWidget->columnWidth(i) + 10);
        ui->treeWidget->sortByColumn(i, Qt::AscendingOrder);
    }
    ui->treeWidget->sortByColumn(0, Qt::AscendingOrder);

    // TODO: This code uses the font height as an approximation for the tree
    // widget row height.  That's not quite right, but it seems to work OK.
    // Do something better?
    ui->treeWidget->setSortingEnabled(true);
    int preferredSize = ui->treeWidget->fontMetrics().height() * data.size();
    resize(width(), height() + preferredSize);
}

NavTableWindow::~NavTableWindow()
{
    delete ui;
}

static QList<QVariant> treeItemData(QTreeWidgetItem *item)
{
    QList<QVariant> data;
    data << item->data(0, Qt::UserRole);
    for (int i = 0; i < item->columnCount(); ++i) {
        data << item->data(i, Qt::DisplayRole);
    }
    return data;
}

void NavTableWindow::on_treeWidget_itemActivated(QTreeWidgetItem *item, int column)
{
    supplier->activate(treeItemData(item));
}

void NavTableWindow::on_treeWidget_itemSelectionChanged()
{
    QList<QTreeWidgetItem*> selection = ui->treeWidget->selectedItems();
    if (selection.size() == 1) {
        supplier->select(treeItemData(selection[0]));
    }
}
