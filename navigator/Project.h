#ifndef NAV_PROJECT_H
#define NAV_PROJECT_H

#include <QFuture>
#include <QList>
#include <QStringList>
#include <vector>

#include "../libindexdb/IndexDb.h"
#include "File.h"

namespace indexdb {
    class Index;
    class StringTable;
    class Table;
}

namespace Nav {

class Project;
class FileManager;
class Ref;

extern Project *theProject;

class Project
{
public:
    Project(const QString &indexPath);
    ~Project();
    FileManager &fileManager() { return *m_fileManager; }

    QList<Ref> queryReferencesOfSymbol(const QString &symbol);
    void queryAllSymbols(std::vector<const char*> &output);
    QStringList queryAllPaths();
    Ref findSingleDefinitionOfSymbol(const QString &symbol);
    indexdb::ID fileID(const QString &path);
    QString fileName(indexdb::ID fileID);
    const std::vector<Ref> &globalSymbolDefinitions();
    template <typename Func> void queryFileRefs(
            File &file,
            Func callback,
            uint32_t firstLine = 0,
            uint32_t lastLine = UINT32_MAX);
    indexdb::ID querySymbolType(indexdb::ID symbolID);
    indexdb::ID getSymbolTypeID(const char *symbolType);

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
    indexdb::Table *m_symbolTable;
    indexdb::Table *m_symbolTypeIndexTable;
    indexdb::Table *m_globalSymbolTable;
    QFuture<std::vector<Ref>*> m_globalSymbolDefinitions;
    std::vector<indexdb::ID> m_symbolType;
};

// Reference table
enum RefColumn {
    RC_File         = 0,
    RC_Line         = 1,
    RC_StartColumn  = 2,
    RC_EndColumn    = 3,
    RC_Symbol       = 4,
    RC_RefType      = 5,
    RC_Count        = 6
};

// ReferenceIndex table
enum RefIndexColumn {
    RIC_Symbol      = 0,
    RIC_RefType     = 1,
    RIC_File        = 2,
    RIC_Line        = 3,
    RIC_StartColumn = 4,
    RIC_EndColumn   = 5,
    RIC_Count       = 6
};

// Symbol table
enum SymbolColumn {
    SC_Symbol       = 0,
    SC_SymbolType   = 1,
    SC_Count        = 2
};

template <typename Func>
void Project::queryFileRefs(
        File &file,
        Func callback,
        uint32_t firstLine,
        uint32_t lastLine)
{
    indexdb::Row rowLookup(2);
    assert(RC_File == 0);
    assert(RC_Line == 1);
    rowLookup[RC_File] = fileID(file.path());
    rowLookup[RC_Line] = firstLine;
    // TODO: Add a class named TableIteratorRange (or TableRange) and a method
    // that accepts a lower bound and an upper bound.  It should do a single
    // O(log n) binary search, but produce two iterators.  This will
    // drastically simplify all of the querying code in this class.
    indexdb::TableIterator it = m_refTable->lowerBound(rowLookup);

    indexdb::TableIterator itEnd = m_refTable->end();
    indexdb::Row rowItem(RC_Count);
    for (; it != itEnd; ++it) {
        it.value(rowItem);
        // TODO: See above comment regarding TableIteratorRange.  This ought to
        // be removed.
        if (rowItem[RC_File] != rowLookup[RC_File] ||
                rowItem[RC_Line] > lastLine)
            break;
        Ref ref(*this,
                rowItem[RC_Symbol],
                rowItem[RC_File],
                rowItem[RC_Line],
                rowItem[RC_StartColumn],
                rowItem[RC_EndColumn],
                rowItem[RC_RefType]);
        callback(ref);
    }
}

} // namespace Nav

#endif // NAV_PROJECT_H
