#include "Project.h"

#include <QTime>
#include <QDebug>

#include <QFileInfo>
#include <QtConcurrentRun>

#include "FileManager.h"
#include "File.h"
#include "Misc.h"
#include "Ref.h"
#include "../libindexdb/IndexDb.h"

namespace Nav {

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

Project *theProject;

Project::Project(const QString &path)
{
    m_index = new indexdb::Index(path.toStdString());
    m_symbolStringTable = m_index->stringTable("Symbol");
    m_symbolTypeStringTable = m_index->stringTable("SymbolType");
    m_refTypeStringTable = m_index->stringTable("ReferenceType");
    m_refTable = m_index->table("Reference");
    m_refIndexTable = m_index->table("ReferenceIndex");
    m_symbolTypeIndexTable = m_index->table("SymbolTypeIndex");
    assert(m_symbolStringTable != NULL);
    assert(m_symbolTypeStringTable != NULL);
    assert(m_refTypeStringTable != NULL);
    assert(m_refTable != NULL);
    assert(m_refIndexTable != NULL);
    assert(m_symbolTypeIndexTable != NULL);

    m_sortedSymbolsInited =
            QtConcurrent::run(this, &Project::initSortedSymbols);

    // Query all the paths, then use that to initialize the FileManager.
    m_fileManager = new FileManager(
                QFileInfo(path).absolutePath(),
                queryAllPaths());
}

Project::~Project()
{
    m_sortedSymbolsInited.waitForFinished();
    delete m_fileManager;
    delete m_index;
}

QList<Ref> Project::queryReferencesOfSymbol(const QString &symbol)
{
    QList<Ref> result;

    indexdb::ID symbolID = m_symbolStringTable->id(symbol.toStdString().c_str());
    if (symbolID == indexdb::kInvalidID)
        return result;

    indexdb::Row rowLookup(1);
    assert(RIC_Symbol == 0);
    rowLookup[RIC_Symbol] = symbolID;

    indexdb::Row rowItem(RIC_Count);
    indexdb::TableIterator itEnd = m_refIndexTable->end();
    indexdb::TableIterator it = m_refIndexTable->lowerBound(rowLookup);
    for (; it != itEnd; ++it) {
        it.value(rowItem);
        if (rowLookup[RIC_Symbol] != rowItem[RIC_Symbol])
            break;

        indexdb::ID fileID = rowItem[RIC_File];
        int line = rowItem[RIC_Line];
        int column = rowItem[RIC_StartColumn];
        indexdb::ID kindID = rowItem[RIC_RefType];

        result << Ref(*this,
                      symbolID,
                      fileID,
                      line,
                      column,
                      kindID);
    }

    return result;
}

QStringList Project::querySymbolsAtLocation(File *file, int line, int column)
{
    QStringList result;

    indexdb::ID fileID = this->fileID(file->path());
    if (fileID == indexdb::kInvalidID)
        return result;

    indexdb::Row rowLookup(3);
    assert(RC_File < 3);
    assert(RC_Line < 3);
    assert(RC_StartColumn < 3);
    rowLookup[RC_File] = fileID;
    rowLookup[RC_Line] = line;
    rowLookup[RC_StartColumn] = column;

    indexdb::TableIterator itEnd = m_refTable->end();
    indexdb::TableIterator it = m_refTable->lowerBound(rowLookup);
    for (; it != itEnd; ++it) {
        indexdb::Row rowItem(RC_Count);
        it.value(rowItem);
        if (rowLookup[RC_File] != rowItem[RC_File] ||
                rowLookup[RC_Line] != rowItem[RC_Line] ||
                rowLookup[RC_StartColumn] != rowItem[RC_StartColumn])
            break;
        result << m_symbolStringTable->item(rowItem[RC_Symbol]);
    }

    return result;
}

// Sorting the symbols is slow (e.g. 800ms for LLVM+Clang), so do it just once,
// when the navigator starts up, and start doing it in the background when the
// application starts.
//
// XXX: Consider sorting the symbols in the index instead.
// XXX: It's probably possible to write a parallel_sort that would be a
// drop-in replacement for std::sort.
void Project::initSortedSymbols()
{
    m_sortedSymbols.resize(m_symbolStringTable->size());
    for (uint32_t i = 0, iEnd = m_symbolStringTable->size(); i < iEnd; ++i) {
        const char *symbol = m_symbolStringTable->item(i);
        m_sortedSymbols[i] = symbol;
    }
    struct ConstCharCompare {
        bool operator()(const char *x, const char *y) {
            return strcmp(x, y) < 0;
        }
    };
    std::sort(m_sortedSymbols.begin(),
              m_sortedSymbols.end(),
              ConstCharCompare());
}

// XXX: This function could return a reference rather than copying the symbol
// list.
void Project::queryAllSymbolsSorted(std::vector<const char*> &output)
{
    m_sortedSymbolsInited.waitForFinished();
    output = m_sortedSymbols;
}

QStringList Project::queryAllPaths()
{
    const indexdb::ID pathTypeID = m_symbolTypeStringTable->id("Path");
    if (pathTypeID == indexdb::kInvalidID)
        return QStringList();
    indexdb::Row row(2);
    row[0] = pathTypeID;
    row[1] = 0;
    indexdb::TableIterator itEnd = m_symbolTypeIndexTable->end();
    indexdb::TableIterator it = m_symbolTypeIndexTable->lowerBound(row);
    QStringList result;
    for (; it != itEnd; ++it) {
        it.value(row);
        if (row[0] != pathTypeID)
            break;
        const char *path = m_symbolStringTable->item(row[1]);
        assert(path[0] == '@');
        result.append(path + 1);
    }
    return result;
}

// Finds the only definition ref (or declaration ref) of the symbol.  If there
// isn't a single such ref, return NULL.
Ref Project::findSingleDefinitionOfSymbol(const QString &symbol)
{
    int declCount = 0;
    int defnCount = 0;
    Ref decl;
    Ref defn;
    QList<Ref> refs = theProject->queryReferencesOfSymbol(symbol);
    for (const Ref &ref : refs) {
        if (declCount < 2 && ref.kind() == "Declaration") {
            declCount++;
            decl = ref;
        }
        if (defnCount < 2 && ref.kind() == "Definition") {
            defnCount++;
            defn = ref;
        }
    }
    if (defnCount == 1) {
        return defn;
    } else if (defnCount == 0 && declCount == 1) {
        return decl;
    } else {
        return Ref();
    }
}

QList<Ref> Project::queryAllSymbolDefinitions()
{
    QList<Ref> result;

    indexdb::ID defnKindID = m_refTypeStringTable->id("Definition");
    indexdb::TableIterator itEnd = m_refIndexTable->end();
    indexdb::TableIterator it = m_refIndexTable->begin();

    for (; it != itEnd; ++it) {
        indexdb::Row rowItem(RIC_Count);
        it.value(rowItem);
        if (rowItem[RIC_RefType] != defnKindID)
            continue;
        indexdb::ID symbolID = rowItem[RIC_Symbol];
        indexdb::ID fileID = rowItem[RIC_File];
        int line = rowItem[RIC_Line];
        int column = rowItem[RIC_StartColumn];
        indexdb::ID kindID = rowItem[RIC_RefType];

        result << Ref(*this, symbolID, fileID, line, column, kindID);
    }

    return result;
}

indexdb::ID Project::fileID(const QString &path)
{
    QString symbol = "@" + path;
    return m_symbolStringTable->id(symbol.toStdString().c_str());
}

QString Project::fileName(indexdb::ID fileID)
{
    const char *name = m_symbolStringTable->item(fileID);
    assert(name[0] == '@');
    return name + 1;
}

} // namespace Nav
