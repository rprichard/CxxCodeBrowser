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
    QList<QList<QString> > data = supplier->getData();
    foreach (const QList<QString> &row, data) {
        QTreeWidgetItem *item = new QTreeWidgetItem(row);
        if (row.size() >= 2)
            item->setTextAlignment(1, Qt::AlignRight);
        ui->treeWidget->addTopLevelItem(item);
    }

    for (int i = 0; i < columnLabels.size(); ++i) {
        ui->treeWidget->resizeColumnToContents(i);
        ui->treeWidget->setColumnWidth(
                    i, ui->treeWidget->columnWidth(i) + 10);
    }

    ui->treeWidget->setSortingEnabled(true);
    int preferredSize = ui->treeWidget->fontMetrics().height() * data.size();
    resize(width(), height() + preferredSize);
}

NavTableWindow::~NavTableWindow()
{
    delete ui;
}

void NavTableWindow::on_treeWidget_itemActivated(QTreeWidgetItem *item, int column)
{
    supplier->activate(item->text(0));
}

void NavTableWindow::on_treeWidget_itemSelectionChanged()
{
    QList<QTreeWidgetItem*> selection = ui->treeWidget->selectedItems();
    if (selection.size() == 1) {
        supplier->select(selection[0]->text(0));
    }
}
