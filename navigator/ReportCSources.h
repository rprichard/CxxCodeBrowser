#ifndef NAV_REPORTCSOURCES_H
#define NAV_REPORTCSOURCES_H

#include "TreeReport.h"
#include <QString>
#include <QStringList>
#include <QList>
#include <QVariant>

namespace Nav {

class Project;

class ReportCSources : public Nav::TableReport
{
public:
    ReportCSources(Project *project);

    QString getTitle();
    QStringList getColumns();
    int getRowCount();
    QList<QVariant> getText(const Index &index);
    void select(const Index &index);

private:
    Project *project;
};

} // namespace Nav

#endif // NAV_REPORTCSOURCES_H
