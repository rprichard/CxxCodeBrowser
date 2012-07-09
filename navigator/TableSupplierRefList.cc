#include "TableSupplierRefList.h"
#include "Symbol.h"
#include "File.h"
#include "NavMainWindow.h"
#include <stdint.h>
#include <QVariant>

namespace Nav {

TableSupplierRefList::TableSupplierRefList(Symbol *symbol) :
    symbol(symbol),
    refList(symbol->refs.values())
{
}

QString TableSupplierRefList::getTitle()
{
    return "References to " + symbol->name;
}

QAbstractItemModel *TableSupplierRefList::model()
{
    return this;
}

void TableSupplierRefList::select(const QModelIndex &index)
{
    const Ref &ref = refList[index.row()];
    theMainWindow->showFile(ref.file->path);
    theMainWindow->selectText(ref.line, ref.column, ref.symbol->name.size());
}

int TableSupplierRefList::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : refList.size();
}

int TableSupplierRefList::columnCount(const QModelIndex &parent) const
{
    return 3;
}

QVariant TableSupplierRefList::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        switch (section) {
        case 0: return QVariant("File");
        case 1: return QVariant("Line");
        case 2: return QVariant("Type");
        }
    }
    return QVariant();
}

QVariant TableSupplierRefList::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole) {
        const Ref &ref = refList[index.row()];
        switch (index.column()) {
        case 0: return QVariant(ref.file->path);
        case 1: return QVariant(ref.line);
        case 2: return QVariant(ref.kind);
        }
    }
    return QVariant();
}

} // namespace Nav
