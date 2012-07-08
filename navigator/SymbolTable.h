#ifndef NAV_SYMBOLTABLE_H
#define NAV_SYMBOLTABLE_H

#include <QHash>

namespace Nav {

class Symbol;

class SymbolTable
{
public:
    SymbolTable();
    ~SymbolTable();
    Symbol *symbol(const QString &name, bool createIfNew = false);

private:
    QHash<QString, Symbol*> symbolMap;
};

} // namespace Nav

#endif // NAV_SYMBOLTABLE_H
