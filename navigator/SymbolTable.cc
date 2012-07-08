#include "SymbolTable.h"
#include "Symbol.h"
#include "Misc.h"

namespace Nav {

SymbolTable::SymbolTable()
{
}

SymbolTable::~SymbolTable()
{
    foreach (Symbol *symbol, symbolMap) {
        delete symbol;
    }
}

Symbol *SymbolTable::symbol(const QString &name, bool createIfNew)
{
    if (createIfNew && !symbolMap.contains(name)) {
        symbolMap.insert(name, new Symbol(name));
    }
    return symbolMap.value(name);
}

} // namespace Nav
