#include "Program.h"
#include "Source.h"
#include "Misc.h"

namespace Nav {

Program *theProgram;

Program::Program()
{
}

Program::~Program()
{
    Nav::deleteAll(sources);
}

} // namespace Nav
