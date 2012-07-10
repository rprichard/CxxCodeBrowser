#include "TableWindow.h"
#include "ui_TableWindow.h"
#include "TableSupplier.h"
#include <QDebug>
#include <QKeySequence>
#include <QShortcut>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QTreeWidgetItem>
#include <cstdlib>

namespace Nav {

TableWindow::TableWindow(Nav::TableSupplier *supplier, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TableWindow),
    supplier(supplier)
{
    ui->setupUi(this);

    setWindowTitle(supplier->getTitle());

    // Register Ctrl+Q.
    QShortcut *shortcut = new QShortcut(QKeySequence("Ctrl+Q"), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(close()));

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(supplier->model());

    QItemSelectionModel *selectionModel = ui->treeView->selectionModel();
    ui->treeView->setModel(proxyModel);
    delete selectionModel;
    connect(ui->treeView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this,
            SLOT(selectionChanged()));

    // Fixup the column widths.
    for (int i = 0; i < supplier->model()->columnCount(); ++i) {
        ui->treeView->resizeColumnToContents(i);
        ui->treeView->setColumnWidth(
                    i, ui->treeView->columnWidth(i) + 10);
    }

    // Sort the tree view arbitrarily.
    for (int i = 0; i < supplier->model()->columnCount(); ++i) {
        ui->treeView->sortByColumn(i, Qt::AscendingOrder);
    }
    ui->treeView->sortByColumn(0, Qt::AscendingOrder);
    ui->treeView->setSortingEnabled(true);

    // TODO: This code uses the font height as an approximation for the tree
    // widget row height.  That's not quite right, but it seems to work OK.
    // Do something better?
    // TODO: The rowCount() code won't work for trees.
    int preferredSize = ui->treeView->fontMetrics().height() * ui->treeView->model()->rowCount();
    resize(width(), height() + preferredSize);
}

TableWindow::~TableWindow()
{
    delete ui;
}

void TableWindow::selectionChanged()
{
    QModelIndexList selection = ui->treeView->selectionModel()->selectedRows();
    if (selection.size() == 1) {
        supplier->select(proxyModel->mapToSource(selection[0]));
    }
}

void TableWindow::on_treeView_activated(const QModelIndex &index)
{
    supplier->activate(proxyModel->mapToSource(index));
}

} // namespace Nav
