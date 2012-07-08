#include "Project.h"
#include "CSource.h"
#include "FileManager.h"
#include "Misc.h"

namespace Nav {

Project *theProject;

Project::Project() : fileManager(new FileManager)
{
}

Project::~Project()
{
    Nav::deleteAll(csources);
    delete fileManager;
}

} // namespace Nav
