#include "TableSupplierSourceList.h"
#include "Project.h"
#include "CSource.h"
#include <iostream>
#include "NavMainWindow.h"

namespace Nav {

QString TableSupplierSourceList::getTitle()
{
    return "C/C++ Translation Units";
}

QStringList TableSupplierSourceList::getColumnLabels()
{
    QStringList result;
    result << "Source Path";
    return result;
}

QList<QList<QString> > TableSupplierSourceList::getData()
{
    QList<QList<QString> > result;
    foreach (CSource *source, theProject->csources) {
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
