#ifndef NAV_PROJECT_H
#define NAV_PROJECT_H

#include <QFuture>
#include <QList>
#include <QStringList>

namespace indexdb {
    class Index;
    class StringTable;
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
    FileManager &fileManager() { return *m_fileManager; }

    QList<Ref> queryReferencesOfSymbol(const QString &symbol);
    QStringList querySymbolsAtLocation(File *file, int line, int column);
    void queryAllSymbolsSorted(std::vector<const char*> &output);
    QList<File*> queryAllFiles();
    Ref findSingleDefinitionOfSymbol(const QString &symbol);
    QList<Ref> queryAllSymbolDefinitions();

    indexdb::StringTable &symbolStringTable() { return *m_symbolStringTable; }
    indexdb::StringTable &pathStringTable() { return *m_pathStringTable; }
    indexdb::StringTable &referenceTypeStringTable() {
        return *m_referenceTypeStringTable;
    }

private:
    void initSortedSymbols();

private:
    QFuture<void> m_sortedSymbolsInited;
    std::vector<const char*> m_sortedSymbols;
    FileManager *m_fileManager;
    indexdb::Index *m_index;
    indexdb::StringTable *m_symbolStringTable;
    indexdb::StringTable *m_pathStringTable;
    indexdb::StringTable *m_referenceTypeStringTable;
};

} // namespace Nav

#endif // NAV_PROJECT_H
