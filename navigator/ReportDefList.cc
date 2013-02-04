#include "ReportDefList.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <cassert>
#include <string>

#include "MainWindow.h"
#include "Project.h"
#include "Ref.h"
#include "TableReport.h"

namespace Nav {

ReportDefList::ReportDefList(Project &project, QObject *parent) :
    TableReport(parent),
    m_defList(project.globalSymbolDefinitions())
{
}

QString ReportDefList::title()
{
    return "Global Definitions";
}

QStringList ReportDefList::columns()
{
    QStringList ret;
    ret << "Symbol";
    ret << "Path";
    return ret;
}

int ReportDefList::rowCount()
{
    return m_defList.size();
}

const char *ReportDefList::text(int row, int column, std::string &tempBuf)
{
    assert(static_cast<size_t>(row) < m_defList.size());
    if (column == 0) {
        return m_defList[row].symbolCStr();
    } else {
        return m_defList[row].fileNameCStr();
    }
}

void ReportDefList::select(int row)
{
    assert(static_cast<size_t>(row) < m_defList.size());
    theMainWindow->navigateToRef(m_defList[row]);
}

int ReportDefList::compare(int row1, int row2, int col)
{
    assert(static_cast<size_t>(row1) < m_defList.size());
    assert(static_cast<size_t>(row2) < m_defList.size());
    if (col == 0) {
        return static_cast<int>(m_defList[row1].symbolID()) -
                static_cast<int>(m_defList[row2].symbolID());
    } else {
        return static_cast<int>(m_defList[row1].fileID()) -
                static_cast<int>(m_defList[row2].fileID());
    }
}

} // namespace Nav
