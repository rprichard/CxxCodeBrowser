#ifndef NAV_TABLEREPORT_H
#define NAV_TABLEREPORT_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <string>

namespace Nav {

class Regex;

class TableReport : public QObject
{
    Q_OBJECT
public:
    explicit TableReport(QObject *parent = NULL) : QObject(parent) {}
    virtual ~TableReport()              {}
    virtual QString title()             { return "Table"; }
    virtual QStringList columns()       { return QStringList(""); }
    virtual int rowCount() = 0;
    virtual const char *text(int row, int column, std::string &tempBuf) = 0;
    virtual void select(int row)        {}
    virtual bool activate(int row)      { return false; }
    virtual int compare(int row1, int row2, int col);
    virtual bool filter(int row, const Regex &regex, std::string &tempBuf);
};

} // namespace Nav

#endif // NAV_TABLEREPORT_H
