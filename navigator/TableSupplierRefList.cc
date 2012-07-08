#include "TableSupplierRefList.h"
#include "Symbol.h"
#include "File.h"
#include "NavMainWindow.h"
#include <stdint.h>
#include <QVariant>

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

QList<QList<QVariant> > TableSupplierRefList::getData()
{
    QList<QList<QVariant> > result;
    foreach (const Ref &ref, symbol->refs) {
        QList<QVariant> row;
        row << QVariant(reinterpret_cast<unsigned long long>(&ref));
        row << QVariant(ref.file->path);
        row << QVariant(ref.line);
        row << QVariant(ref.kind);
        result << row;
    }
    return result;
}

void TableSupplierRefList::select(const QList<QVariant> &entry)
{
    // TODO: We need to do something better than casting Ref* to and from
    // long long.
    Ref *ref = reinterpret_cast<Ref*>(entry[0].toULongLong());
    theMainWindow->showFile(ref->file->path);
    theMainWindow->selectText(ref->line, ref->column, ref->symbol->name.size());
}

} // namespace Nav
