#ifndef NAV_PROJECT_H
#define NAV_PROJECT_H

#include <QList>
#include <QStringList>

namespace indexdb {
    class Index;
}

namespace Nav {

class Project;
class FileManager;
class File;
class Ref;

extern Project *theProject;

class Project
{
public:
    Project(const QString &indexPath);
    ~Project();
    FileManager *fileManager() { return m_fileManager; }

    QList<Ref> queryReferencesOfSymbol(const QString &symbol);
    QStringList querySymbolsAtLocation(File *file, int line, int column);
    QStringList queryAllSymbols();
    QList<File*> queryAllFiles();

private:
    FileManager *m_fileManager;
    indexdb::Index *m_index;
};

} // namespace Nav

#endif // NAV_PROJECT_H
