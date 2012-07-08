#include "TableSupplierSourceList.h"
#include "Project.h"
#include "CSource.h"
#include <iostream>
#include "NavMainWindow.h"
#include <QVariant>

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

QList<QList<QVariant> > TableSupplierSourceList::getData()
{
    QList<QList<QVariant> > result;
    foreach (CSource *source, theProject->csources) {
        QList<QVariant> row;
        row << QVariant("");
        row << QVariant(source->path);
        result << row;
    }
    return result;
}

void TableSupplierSourceList::select(const QList<QVariant> &entry)
{
    theMainWindow->showFile(entry[1].toString());
}

} // namespace Nav
