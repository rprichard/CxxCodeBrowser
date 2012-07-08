#ifndef NAV_TABLESUPPLIER_H
#define NAV_TABLESUPPLIER_H

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>

namespace Nav {

class TableSupplier
{
public:
    virtual QString getTitle() = 0;
    virtual QStringList getColumnLabels() = 0;
    virtual QList<QList<QVariant> > getData() = 0;
    virtual void select(const QList<QVariant> &entry) {}
    virtual void activate(const QList<QVariant> &entry) {}
};

} // namespace Nav

#endif // NAV_TABLESUPPLIER_H
