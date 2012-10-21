#include "ReportDefList.h"

#include "MainWindow.h"
#include "Project.h"
#include "Ref.h"

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
    if (column == 0) {
        return m_defList[row].symbolCStr();
    } else {
        return m_defList[row].fileNameCStr();
    }
}

void ReportDefList::select(int row)
{
    theMainWindow->navigateToRef(m_defList[row]);
}

int ReportDefList::compare(int row1, int row2, int col)
{
    if (col == 0) {
        return static_cast<int>(m_defList[row1].symbolID()) -
                static_cast<int>(m_defList[row2].symbolID());
    } else {
        return static_cast<int>(m_defList[row1].fileID()) -
                static_cast<int>(m_defList[row2].fileID());
    }
}

} // namespace Nav
