#include "ReportFileList.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <cassert>
#include <string>

#include "File.h"
#include "FileManager.h"
#include "MainWindow.h"
#include "Project.h"
#include "TableReport.h"

namespace Nav {

ReportFileList::ReportFileList(Project &project, QObject *parent) :
    TableReport(parent),
    m_project(project)
{
    m_paths = m_project.queryAllPaths();
}

QString ReportFileList::title()
{
    return "Files";
}

QStringList ReportFileList::columns()
{
    return QStringList("Path");
}

int ReportFileList::rowCount()
{
    return m_paths.size();
}

const char *ReportFileList::text(int row, int column, std::string &tempBuf)
{
    assert(row >= 0 && row < m_paths.size());
    tempBuf = m_paths[row].toStdString();
    return tempBuf.c_str();
}

void ReportFileList::select(int row)
{
    assert(row >= 0 && row < m_paths.size());
    File &file = m_project.fileManager().file(m_paths[row]);
    theMainWindow->navigateToFile(&file);
}

} // namespace Nav
