#ifndef NAV_PROJECT_H
#define NAV_PROJECT_H

#include <QFuture>
#include <QList>
#include <QStringList>
#include <vector>

#include "../libindexdb/IndexDb.h"

namespace indexdb {
    class Index;
    class StringTable;
    class Table;
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
    void queryAllSymbols(std::vector<const char*> &output);
    QStringList queryAllPaths();
    Ref findSingleDefinitionOfSymbol(const QString &symbol);
    indexdb::ID fileID(const QString &path);
    QString fileName(indexdb::ID fileID);
    const std::vector<Ref> &globalSymbolDefinitions();

    indexdb::StringTable &symbolStringTable() { return *m_symbolStringTable; }
    indexdb::StringTable &refTypeStringTable() {
        return *m_refTypeStringTable;
    }

private:
    std::vector<Ref> *queryGlobalSymbolDefinitions();

private:
    FileManager *m_fileManager;
    indexdb::Index *m_index;
    indexdb::StringTable *m_symbolStringTable;
    indexdb::StringTable *m_symbolTypeStringTable;
    indexdb::StringTable *m_refTypeStringTable;
    indexdb::Table *m_refTable;
    indexdb::Table *m_refIndexTable;
    indexdb::Table *m_symbolTypeIndexTable;
    indexdb::Table *m_globalSymbolTable;
    QFuture<std::vector<Ref>*> m_globalSymbolDefinitions;
};

} // namespace Nav

#endif // NAV_PROJECT_H
