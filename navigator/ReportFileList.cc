#include "ReportFileList.h"

#include "File.h"
#include "FileManager.h"
#include "Project.h"
#include "MainWindow.h"

namespace Nav {

ReportFileList::ReportFileList(Project *project) : m_project(project)
{
    m_paths = m_project->queryAllPaths();
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
    return m_paths.size();
}

QList<QVariant> ReportFileList::getText(const Index &index)
{
    QList<QVariant> result;
    result << m_paths[index.row()];
    return result;
}

void ReportFileList::select(const Index &index)
{
    File &file = m_project->fileManager().file(m_paths[index.row()]);
    theMainWindow->navigateToFile(&file);

}

} // namespace Nav
