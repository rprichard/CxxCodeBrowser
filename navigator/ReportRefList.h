#ifndef NAV_REPORTREFLIST_H
#define NAV_REPORTREFLIST_H

#include "TreeReport.h"
#include "Ref.h"
#include <QList>
#include <QVariant>

namespace Nav {

class Symbol;

class ReportRefList : public TableReport
{
public:
    ReportRefList(Symbol *symbol);

    QString getTitle();
    QStringList getColumns();
    int getRowCount();
    QList<QVariant> getText(const Index &index);
    void select(const Index &index);

private:
    Symbol *symbol;
    QList<Ref> refList;
};

} // namespace Nav

#endif // NAV_REPORTREFLIST_H
