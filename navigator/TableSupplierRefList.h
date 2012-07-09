#ifndef NAV_TABLESUPPLIERREFLIST_H
#define NAV_TABLESUPPLIERREFLIST_H

#include "TableSupplier.h"
#include "Ref.h"
#include <QList>
#include <QString>
#include <QStringList>
#include <QAbstractTableModel>

namespace Nav {

class Symbol;

class TableSupplierRefList : public QAbstractTableModel, public TableSupplier
{
    Q_OBJECT

public:
    explicit TableSupplierRefList(Symbol *symbol);

    // TableSupplier
    virtual QString getTitle();
    virtual QAbstractItemModel *model();
    virtual void select(const QModelIndex &index);

    // QAbstractTableModel
    virtual int rowCount(const QModelIndex &parent) const;
    virtual int columnCount(const QModelIndex &parent) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual QVariant data(const QModelIndex &index, int role) const;

private:
    Symbol *symbol;
    QList<Ref> refList;
};

} // namespace Nav

#endif // NAV_TABLESUPPLIERREFLIST_H
