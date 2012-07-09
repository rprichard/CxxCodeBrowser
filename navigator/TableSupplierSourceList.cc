#include "TableSupplierSourceList.h"
#include "Project.h"
#include "CSource.h"
#include <iostream>
#include "NavMainWindow.h"
#include <QVariant>

namespace Nav {

QString TableSupplierSourceList::getTitle()
{
    return "C/C++ Translation Units";
}

QAbstractItemModel *TableSupplierSourceList::model()
{
    return this;
}

void TableSupplierSourceList::select(const QModelIndex &index)
{
    theMainWindow->showFile(theProject->csources[index.row()]->path);
}

int TableSupplierSourceList::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : theProject->csources.size();
}

int TableSupplierSourceList::columnCount(const QModelIndex &parent) const
{
    return 1;
}

QVariant TableSupplierSourceList::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && section == 0) {
        return QVariant("Source Path");
    }
    return QVariant();
}

QVariant TableSupplierSourceList::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole) {
        return QVariant(theProject->csources[index.row()]->path);
    }
    return QVariant();
}

} // namespace Nav
