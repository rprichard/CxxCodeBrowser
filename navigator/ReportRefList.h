#ifndef NAV_REPORTREFLIST_H
#define NAV_REPORTREFLIST_H

#include "TreeReport.h"
#include "Ref.h"
#include <QList>
#include <QVariant>
#include <QString>

namespace Nav {

class Project;
class Symbol;

class ReportRefList : public TableReport
{
public:
    ReportRefList(Project *project, const QString &symbol);

    QString getTitle();
    QStringList getColumns();
    int getRowCount();
    QList<QVariant> getText(const Index &index);
    void select(const Index &index);

private:
    QString m_symbol;
    QList<Ref> m_refList;
};

} // namespace Nav

#endif // NAV_REPORTREFLIST_H
