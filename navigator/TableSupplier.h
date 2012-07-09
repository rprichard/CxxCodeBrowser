#ifndef NAV_TABLESUPPLIER_H
#define NAV_TABLESUPPLIER_H

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QObject>

class QAbstractItemModel;
class QModelIndex;

namespace Nav {

class TableSupplier
{
public:
    virtual ~TableSupplier() {}
    virtual QString getTitle() = 0;
    virtual QAbstractItemModel *model() = 0;
    virtual void select(const QModelIndex &index) {}
    virtual void activate(const QModelIndex &index) {}
};

} // namespace Nav

#endif // NAV_TABLESUPPLIER_H
