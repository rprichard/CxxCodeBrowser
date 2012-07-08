#ifndef NAV_TABLESUPPLIERSOURCELIST_H
#define NAV_TABLESUPPLIERSOURCELIST_H

#include "TableSupplier.h"

namespace Nav {

class TableSupplierSourceList : public TableSupplier
{
public:
    virtual QString getTitle();
    virtual QStringList getColumnLabels();
    virtual QList<QList<QString> > getData();
    virtual void select(const QString &entry);
};

} // namespace Nav

#endif // NAV_TABLESUPPLIERSOURCELIST_H
