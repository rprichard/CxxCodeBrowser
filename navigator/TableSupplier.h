#ifndef NAV_TABLESUPPLIER_H
#define NAV_TABLESUPPLIER_H

#include <QList>
#include <QString>
#include <QStringList>

namespace Nav {

class TableSupplier
{
public:
    virtual QStringList getColumnLabels() = 0;
    virtual QList<QList<QString> > getData() = 0;
    virtual void select(const QString &entry) {}
    virtual void activate(const QString &entry) {}
};

} // namespace Nav

#endif // NAV_TABLESUPPLIER_H
