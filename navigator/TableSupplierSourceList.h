#ifndef NAV_TABLESUPPLIERSOURCELIST_H
#define NAV_TABLESUPPLIERSOURCELIST_H

#include "TableSupplier.h"
#include <QAbstractTableModel>

namespace Nav {

class TableSupplierSourceList : public QAbstractTableModel, public TableSupplier
{
    Q_OBJECT

public:
    // TableSupplier
    virtual QString getTitle();
    virtual QAbstractItemModel *model();
    virtual void select(const QModelIndex &index);

    // QAbstractTableModel
    virtual int rowCount(const QModelIndex &parent) const;
    virtual int columnCount(const QModelIndex &parent) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
};

} // namespace Nav

#endif // NAV_TABLESUPPLIERSOURCELIST_H
