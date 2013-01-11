#ifndef NAV_REPORTREFLIST_H
#define NAV_REPORTREFLIST_H

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <string>

#include "Ref.h"
#include "TableReport.h"

namespace Nav {

class Project;
class Symbol;

class ReportRefList : public TableReport
{
    Q_OBJECT
public:
    ReportRefList(
            Project &project,
            const QString &symbol,
            QObject *parent = NULL);
    QString title();
    QStringList columns();
    int rowCount();
    const char *text(int row, int col, std::string &tempBuf);
    void select(int row);
    int compare(int row1, int row2, int col);

private:
    QString m_symbol;
    QList<Ref> m_refList;
};

} // namespace Nav

#endif // NAV_REPORTREFLIST_H
