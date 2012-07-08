#ifndef NAV_REF_H
#define NAV_REF_H

#include <QString>

namespace Nav {

class Symbol;
class File;

class Ref
{
public:
    Symbol *symbol;
    File *file;
    int line;
    int column;
    QString kind;
};

unsigned int qHash(const Nav::Ref &ref);

bool operator==(const Nav::Ref &r1, const Nav::Ref &r2);

} // namespace Nav

#endif // NAV_REF_H
