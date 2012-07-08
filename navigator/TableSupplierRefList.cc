#include "TableSupplierRefList.h"
#include "Symbol.h"
#include "File.h"

namespace Nav {

TableSupplierRefList::TableSupplierRefList(Symbol *symbol) : symbol(symbol)
{
}

QString TableSupplierRefList::getTitle()
{
    return "References to " + symbol->name;
}

QStringList TableSupplierRefList::getColumnLabels()
{
    QStringList result;
    result << "File";
    result << "Line";
    result << "Type";
    return result;
}

QList<QList<QString> > TableSupplierRefList::getData()
{
    QList<QList<QString> > result;
    foreach (const Ref &ref, symbol->refs) {
        QList<QString> row;
        row << ref.file->path;
        row << QString::number(ref.line);
        row << ref.kind;
        result << row;
    }
    return result;
}

void TableSupplierRefList::select(const QString &entry)
{
}

} // namespace Nav
