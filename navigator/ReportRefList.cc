#include "File.h"
#include "MainWindow.h"
#include "Project.h"
#include "Ref.h"
#include "ReportRefList.h"

namespace Nav {

ReportRefList::ReportRefList(Project *project, const QString &symbol) :
    m_symbol(symbol)
{
    m_refList = project->queryReferencesOfSymbol(symbol);
}

QString ReportRefList::getTitle()
{
    return "References to " + m_symbol;
}

QStringList ReportRefList::getColumns()
{
    QStringList result;
    result << "File";
    result << "Line";
    result << "Column";
    result << "Type";
    return result;
}

int ReportRefList::getRowCount()
{
    return m_refList.size();
}

QList<QVariant> ReportRefList::getText(const Index &index)
{
    QList<QVariant> result;
    const Ref &ref = m_refList[index.row()];
    result << ref.file->path();
    result << ref.line;
    result << ref.column;
    result << ref.kind;
    return result;
}

void ReportRefList::select(const Index &index)
{
    const Ref &ref = m_refList[index.row()];
    theMainWindow->showFile(ref.file->path());
    theMainWindow->selectText(ref.line, ref.column, 1);
}

} // namespace Nav
