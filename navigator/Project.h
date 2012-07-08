#ifndef NAV_PROJECT_H
#define NAV_PROJECT_H

#include <QList>

namespace Nav {

class Project;
class CSource;
class FileManager;

extern Project *theProject;

class Project
{
public:
    Project();
    ~Project();
    QList<CSource*> csources;
    FileManager *fileManager;
};

} // namespace Nav

#endif // NAV_PROJECT_H
