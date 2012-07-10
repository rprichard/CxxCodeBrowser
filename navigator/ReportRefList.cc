#include "ReportRefList.h"
#include "Symbol.h"
#include "File.h"
#include "MainWindow.h"

namespace Nav {

ReportRefList::ReportRefList(Symbol *symbol) :
    symbol(symbol),
    refList(symbol->refs.values())
{
}

QString ReportRefList::getTitle()
{
    return "References to " + symbol->name;
}

QStringList ReportRefList::getColumns()
{
    QStringList result;
    result << "File";
    result << "Line";
    result << "Type";
    return result;
}

int ReportRefList::getRowCount()
{
    return refList.size();
}

QList<QVariant> ReportRefList::getText(const Index &index)
{
    QList<QVariant> result;
    result << refList[index.row()].file->path;
    result << refList[index.row()].line;
    result << refList[index.row()].kind;
    return result;
}

void ReportRefList::select(const Index &index)
{
    const Ref &ref = refList[index.row()];
    theMainWindow->showFile(ref.file->path);
    theMainWindow->selectText(ref.line, ref.column, ref.symbol->name.size());
}

} // namespace Nav
