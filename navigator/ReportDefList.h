#ifndef NAV_REPORTDEFLIST_H
#define NAV_REPORTDEFLIST_H

#include <QObject>
#include <QSize>
#include <QString>
#include <QStringList>
#include <string>
#include <vector>

#include "TableReport.h"

namespace Nav {

class Project;
class Ref;

const QSize kReportDefListDefaultSize(600, 800);

class ReportDefList : public TableReport
{
    Q_OBJECT
public:
    ReportDefList(Project &project, QObject *parent = NULL);
    QString title();
    QStringList columns();
    int rowCount();
    const char *text(int row, int column, std::string &tempBuf);
    void select(int row);
    bool activate(int row) { select(row); return true; }
    int compare(int row1, int row2, int col);

private:
    const std::vector<Ref> &m_defList;
};

} // namespace Nav

#endif // NAV_REPORTDEFLIST_H
