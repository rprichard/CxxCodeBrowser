#ifndef NAV_REPORTSYMLIST_H
#define NAV_REPORTSYMLIST_H

#include <QObject>
#include <QSize>
#include <QString>
#include <QStringList>
#include <string>

#include "TableReport.h"

namespace Nav {

class Project;

const QSize kReportSymListDefaultSize(600, 800);

class ReportSymList : public TableReport
{
    Q_OBJECT
public:
    ReportSymList(Project &project, QObject *parent = NULL);
    QString title();
    QStringList columns();
    int rowCount();
    const char *text(int row, int column, std::string &tempBuf);
    int compare(int row1, int row2, int col);
    bool activate(int row);

private:
    Project &m_project;
};

} // namespace Nav

#endif // NAV_REPORTSYMLIST_H
