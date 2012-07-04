#include "NavTableWindow.h"
#include "ui_NavTableWindow.h"
#include "TableSupplier.h"
#include <QStringList>
#include <QTreeWidgetItem>
#include <QStandardItemModel>
#include <cstdlib>

static QStringList stringListFromStdVector(
        const std::vector<std::string> &input)
{
    QStringList result;
    for (size_t i = 0; i < input.size(); ++i) {
        result.append(QString::fromStdString(input[i]));
    }
    return result;
}

NavTableWindow::NavTableWindow(Nav::TableSupplier *supplier, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::NavTableWindow),
    supplier(supplier)
{
    ui->setupUi(this);

    std::vector<std::string> columnLabels = supplier->getColumnLabels();
    std::vector<std::vector<std::string> > data = supplier->getData();

    ui->treeWidget->setHeaderLabels(stringListFromStdVector(columnLabels));

    for (size_t i = 0; i < data.size(); ++i) {
        ui->treeWidget->addTopLevelItem(new QTreeWidgetItem(stringListFromStdVector(data[i])));
    }
}

NavTableWindow::~NavTableWindow()
{
    delete ui;
}

void NavTableWindow::on_treeWidget_itemActivated(QTreeWidgetItem *item, int column)
{
    supplier->activate(item->text(0).toStdString());
}


void NavTableWindow::on_treeWidget_itemSelectionChanged()
{
    QList<QTreeWidgetItem*> selection = ui->treeWidget->selectedItems();
    if (selection.size() == 1) {
        supplier->select(selection[0]->text(0).toStdString());
    }
}
