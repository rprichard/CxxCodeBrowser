#include "TableSupplierSourceList.h"
#include "Program.h"
#include "Source.h"
#include <iostream>
#include "NavMainWindow.h"

namespace Nav {

QStringList TableSupplierSourceList::getColumnLabels()
{
    QStringList result;
    result << "Source Path";
    return result;
}

QList<QList<QString> > TableSupplierSourceList::getData()
{
    QList<QList<QString> > result;
    foreach (Source *source, theProgram->sources) {
        QStringList row;
        row << source->path;
        result << row;
    }
    return result;
}

void TableSupplierSourceList::select(const QString &entry)
{
    theMainWindow->showFile(entry);
}

} // namespace Nav
