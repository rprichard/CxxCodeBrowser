#ifndef NAV_TABLESUPPLIERREFLIST_H
#define NAV_TABLESUPPLIERREFLIST_H

#include "TableSupplier.h"
#include <QList>
#include <QString>
#include <QStringList>

namespace Nav {

class Symbol;

class TableSupplierRefList : public TableSupplier
{
public:
    TableSupplierRefList(Symbol *symbol);
    virtual QString getTitle();
    virtual QStringList getColumnLabels();
    virtual QList<QList<QVariant> > getData();
    virtual void select(const QList<QVariant> &entry);

private:
    Symbol *symbol;
};

} // namespace Nav

#endif // NAV_TABLESUPPLIERREFLIST_H
