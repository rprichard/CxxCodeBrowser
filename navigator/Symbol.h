#ifndef NAV_SYMBOL_H
#define NAV_SYMBOL_H

#include <QSet>
#include <QString>
#include "Ref.h"

namespace Nav {

class Symbol
{
public:
    Symbol(const QString &name);
    QString name;
    QSet<Ref> refs;
};

} // namespace Nav

#endif // NAV_SYMBOL_H
