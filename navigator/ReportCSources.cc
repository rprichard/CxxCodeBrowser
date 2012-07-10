#include "ReportCSources.h"
#include "Project.h"
#include "CSource.h"
#include "MainWindow.h"

namespace Nav {

ReportCSources::ReportCSources(Project *project) : project(project)
{
}

QString ReportCSources::getTitle()
{
    return "C/C++ Translation Units";
}

QStringList ReportCSources::getColumns()
{
    return QStringList("Source Path");
}

int ReportCSources::getRowCount()
{
    return project->csources.size();
}

QList<QVariant> ReportCSources::getText(const Index &index)
{
    QList<QVariant> result;
    result << project->csources[index.row()]->path;
    return result;
}

void ReportCSources::select(const Index &index)
{
    theMainWindow->showFile(project->csources[index.row()]->path);
}

} // namespace Nav
