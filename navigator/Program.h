#ifndef NAV_PROGRAM_H
#define NAV_PROGRAM_H

#include <vector>

namespace Nav {

class Program;
class Source;

extern Program *theProgram;

class Program
{
public:
    Program();
    ~Program();
    std::vector<Source*> sources;
};

} // namespace Nav

#endif // NAV_PROGRAM_H
