#include "TableSupplierSourceList.h"
#include "Program.h"
#include "Source.h"
#include <iostream>
#include "NavMainWindow.h"

namespace Nav {

std::vector<std::string> TableSupplierSourceList::getColumnLabels()
{
    std::vector<std::string> result;
    result.push_back("Source Path");
    return result;
}

std::vector<std::vector<std::string> > TableSupplierSourceList::getData()
{
    std::vector<std::vector<std::string> > result;
    for (std::vector<Source*>::iterator it = theProgram->sources.begin();
            it != theProgram->sources.end();
            ++it) {
        std::vector<std::string> row;
        Source *source = *it;
        row.push_back(source->path);
        result.push_back(row);
    }
    return result;
}

void TableSupplierSourceList::select(const std::string &entry)
{
    theMainWindow->showFile(entry);
}

} // namespace Nav
