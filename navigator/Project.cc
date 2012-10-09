#include "Project.h"

#include <QFileInfo>
#include <QtConcurrentRun>

#include "FileManager.h"
#include "File.h"
#include "Misc.h"
#include "Ref.h"
#include "../libindexdb/IndexDb.h"

namespace Nav {

Project *theProject;

Project::Project(const QString &path)
{
    m_index = new indexdb::Index(path.toStdString());
    m_symbolStringTable = m_index->stringTable("Symbol");
    m_symbolTypeStringTable = m_index->stringTable("SymbolType");
    m_refTypeStringTable = m_index->stringTable("ReferenceType");
    m_refTable = m_index->table("Reference");
    m_refIndexTable = m_index->table("ReferenceIndex");
    m_symbolTable = m_index->table("Symbol");
    m_symbolTypeIndexTable = m_index->table("SymbolTypeIndex");
    m_globalSymbolTable = m_index->table("GlobalSymbol");
    assert(m_symbolStringTable != NULL);
    assert(m_symbolTypeStringTable != NULL);
    assert(m_refTypeStringTable != NULL);
    assert(m_refTable != NULL);
    assert(m_refIndexTable != NULL);
    assert(m_symbolTable != NULL);
    assert(m_symbolTypeIndexTable != NULL);

    // Query all the paths, then use that to initialize the FileManager.
    m_fileManager = new FileManager(
                QFileInfo(path).absolutePath(),
                queryAllPaths());

    // Start this query in the background.
    m_globalSymbolDefinitions =
            QtConcurrent::run(this, &Project::queryGlobalSymbolDefinitions);

    // Load the symbol->symbolType map into memory for faster accesses.
    m_symbolType.resize(m_symbolStringTable->size(), indexdb::kInvalidID);
    indexdb::Row symbolRow(SC_Count);
    for (indexdb::TableIterator it = m_symbolTable->begin(),
            itEnd = m_symbolTable->end(); it != itEnd; ++it) {
        it.value(symbolRow);
        m_symbolType[symbolRow[SC_Symbol]] = symbolRow[SC_SymbolType];
    }
}

Project::~Project()
{
    delete m_fileManager;
    delete m_index;
    delete m_globalSymbolDefinitions.result();
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
        int startColumn = rowItem[RIC_StartColumn];
        int endColumn = rowItem[RIC_EndColumn];
        indexdb::ID kindID = rowItem[RIC_RefType];

        result << Ref(*this,
                      symbolID,
                      fileID,
                      line,
                      startColumn,
                      endColumn,
                      kindID);
    }

    return result;
}

QStringList Project::querySymbolsAtLocation(File *file, int line, int column)
{
    QSet<QString> result;

    indexdb::ID fileID = this->fileID(file->path());
    if (fileID == indexdb::kInvalidID)
        return QStringList();

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

    QStringList resultList = result.toList();
    qSort(resultList);
    return resultList;
}

void Project::queryAllSymbols(std::vector<const char*> &output)
{
    output.resize(m_symbolStringTable->size());
    for (uint32_t i = 0, iEnd = m_symbolStringTable->size(); i < iEnd; ++i) {
        const char *symbol = m_symbolStringTable->item(i);
        output[i] = symbol;
    }
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

std::vector<Ref> *Project::queryGlobalSymbolDefinitions()
{
    std::vector<Ref> *ret = new std::vector<Ref>;

    indexdb::ID defnKindID = m_refTypeStringTable->id("Definition");
    indexdb::TableIterator it = m_refIndexTable->begin();
    indexdb::TableIterator itEnd = m_refIndexTable->end();
    indexdb::TableIterator git = m_globalSymbolTable->begin();
    indexdb::TableIterator gitEnd = m_globalSymbolTable->end();
    indexdb::Row rowGlobal(1);
    indexdb::Row rowFilter(RIC_RefType + 1);
    indexdb::Row rowItem(RIC_Count);
    assert(RIC_Symbol < RIC_RefType);

    if (git != gitEnd) {
        git.value(rowGlobal);
        for (; it != itEnd; ++it) {
            it.value(rowFilter);

            // Filter out non-definitions and definitions of non-global
            // symbols.
            if (rowFilter[RIC_RefType] != defnKindID)
                continue;
            while (rowFilter[RIC_Symbol] > rowGlobal[0]) {
                ++git;
                if (git == gitEnd)
                    goto end;
                git.value(rowGlobal);
            }
            if (rowFilter[RIC_Symbol] < rowGlobal[0])
                continue;

            // Record this global symbol definition.
            it.value(rowItem);
            indexdb::ID symbolID = rowItem[RIC_Symbol];
            indexdb::ID fileID = rowItem[RIC_File];
            int line = rowItem[RIC_Line];
            int startColumn = rowItem[RIC_StartColumn];
            int endColumn = rowItem[RIC_EndColumn];
            indexdb::ID kindID = rowItem[RIC_RefType];
            ret->push_back(Ref(*this, symbolID, fileID, line, startColumn, endColumn, kindID));
        }
        end: ;
    }

    return ret;
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

const std::vector<Ref> &Project::globalSymbolDefinitions()
{
    return *m_globalSymbolDefinitions.result();
}

indexdb::ID Project::querySymbolType(indexdb::ID symbolID)
{
    assert(symbolID < m_symbolType.size());
    return m_symbolType[symbolID];
}

indexdb::ID Project::getSymbolTypeID(const char *symbolType)
{
    return m_symbolTypeStringTable->id(symbolType);
}

} // namespace Nav
