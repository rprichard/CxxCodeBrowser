#include "Project.h"
#include "CSource.h"
#include "FileManager.h"
#include "Misc.h"
#include "SymbolTable.h"

namespace Nav {

Project *theProject;

Project::Project() :
    fileManager(new FileManager),
    symbolTable(new SymbolTable)
{
}

Project::~Project()
{
    Nav::deleteAll(csources);
    delete fileManager;
    delete symbolTable;
}

} // namespace Nav
