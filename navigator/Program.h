#ifndef NAV_PROGRAM_H
#define NAV_PROGRAM_H

#include <QList>

namespace Nav {

class Program;
class Source;

extern Program *theProgram;

class Program
{
public:
    Program();
    ~Program();
    QList<Source*> sources;
};

} // namespace Nav

#endif // NAV_PROGRAM_H
