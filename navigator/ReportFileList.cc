#include "ReportFileList.h"
#include "Project.h"
#include "File.h"
#include "MainWindow.h"

namespace Nav {

ReportFileList::ReportFileList(Project *project) : m_project(project)
{
    m_files = m_project->queryAllFiles();
}

QString ReportFileList::getTitle()
{
    return "Files";
}

QStringList ReportFileList::getColumns()
{
    return QStringList("Path");
}

int ReportFileList::getRowCount()
{
    return m_files.size();
}

QList<QVariant> ReportFileList::getText(const Index &index)
{
    QList<QVariant> result;
    result << m_files[index.row()]->path();
    return result;
}

void ReportFileList::select(const Index &index)
{
    theMainWindow->navigateToFile(m_files[index.row()]);
}

} // namespace Nav
