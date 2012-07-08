#include "Project.h"
#include "CSource.h"
#include "Misc.h"

namespace Nav {

Project *theProject;

Project::Project()
{
}

Project::~Project()
{
    Nav::deleteAll(sources);
}

} // namespace Nav
