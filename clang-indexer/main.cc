#include <QtDebug>
#include <QtCore>
#include <cassert>
#include <cstring>
#include <fstream>
#include <vector>
#include <json/reader.h>
#include <clang-c/Index.h>
#include "IndexDb.h"


struct TUIndexer {
    CXChildVisitResult visitor(
            CXCursor cursor,
            CXCursor parent);
    static CXChildVisitResult visitor(
            CXCursor cursor,
            CXCursor parent,
            CXClientData data);

    indexdb::Index *index;

    indexdb::HashSet<char> *pathStringTable;
    indexdb::HashSet<char> *kindStringTable;
    indexdb::HashSet<char> *usrStringTable;

    indexdb::Table *refTable;
    indexdb::Table *locTable;
};

static inline const char *nullToBlank(const char *string)
{
    return (string != NULL) ? string : "";
}

CXChildVisitResult TUIndexer::visitor(
        CXCursor cursor,
        CXCursor parent)
{
    CXString usrCXStr = clang_getCursorUSR(cursor);
    CXCursor cursorRef = clang_getCursorReferenced(cursor);
    if (!clang_Cursor_isNull(cursorRef)) {
        clang_disposeString(usrCXStr);
        usrCXStr = clang_getCursorUSR(cursorRef);
    }

    const char *usrCStr = clang_getCString(usrCXStr);
    assert(usrCStr);
    if (usrCStr[0] != '\0') {
        CXFile file;
        unsigned int line, column, offset;
        clang_getInstantiationLocation(
                clang_getCursorLocation(cursor),
                    &file, &line, &column, &offset);
        CXString kindCXStr = clang_getCursorKindSpelling(clang_getCursorKind(cursor));
        CXString fileNameCXStr = clang_getFileName(file);

        {
            const char *fileNameCStr = nullToBlank(clang_getCString(fileNameCXStr));
            const char *kindCStr = nullToBlank(clang_getCString(kindCXStr));

            indexdb::ID usrID = usrStringTable->id(usrCStr);
            indexdb::ID fileNameID = pathStringTable->id(fileNameCStr);
            indexdb::ID kindID = kindStringTable->id(kindCStr);

            {
                indexdb::Row refRow(5);
                refRow[0] = usrID;
                refRow[1] = fileNameID;
                refRow[2] = line;
                refRow[3] = column;
                refRow[4] = kindID;
                refTable->add(refRow);
            }

            {
                indexdb::Row locRow(4);
                locRow[0] = fileNameID;
                locRow[1] = line;
                locRow[2] = column;
                locRow[3] = usrID;
                locTable->add(locRow);
            }
        }

        clang_disposeString(kindCXStr);
        clang_disposeString(fileNameCXStr);
    }

    clang_disposeString(usrCXStr);

    return CXChildVisit_Recurse;
}

CXChildVisitResult TUIndexer::visitor(
        CXCursor cursor,
        CXCursor parent,
        CXClientData data)
{
    return static_cast<TUIndexer*>(data)->visitor(cursor, parent);
}

struct SourceFileInfo {
    std::string path;
    std::vector<std::string> defines;
    std::vector<std::string> includes;
    std::vector<std::string> extraArgs;
};

indexdb::Index *indexSourceFile(SourceFileInfo sfi)
{
    std::vector<char*> args;
    for (auto define : sfi.defines) {
        std::string arg = "-D" + define;
        args.push_back(strdup(arg.c_str()));
    }
    for (auto include : sfi.includes) {
        std::string arg = "-I" + include;
        args.push_back(strdup(arg.c_str()));
    }
    for (auto arg : sfi.extraArgs) {
        args.push_back(strdup(arg.c_str()));
    }

    CXIndex cxindex = clang_createIndex(0, 0);
    CXTranslationUnit tu = clang_parseTranslationUnit(
                cxindex,
                sfi.path.c_str(),
                args.data(), args.size(),
                NULL, 0,
                CXTranslationUnit_DetailedPreprocessingRecord // CXTranslationUnit_None
                );

    TUIndexer indexer;
    indexer.index = new indexdb::Index;

    indexer.pathStringTable = indexer.index->addStringTable("path");
    indexer.kindStringTable = indexer.index->addStringTable("kind");
    indexer.usrStringTable = indexer.index->addStringTable("usr");

    std::vector<std::string> refColumns;
    refColumns.push_back("usr");
    refColumns.push_back("path");
    refColumns.push_back(""); // line
    refColumns.push_back(""); // column
    refColumns.push_back("kind");
    indexer.refTable = indexer.index->addTable("ref", refColumns);

    std::vector<std::string> locColumns;
    locColumns.push_back("path");
    locColumns.push_back(""); // line
    locColumns.push_back(""); // column
    locColumns.push_back("usr");
    indexer.locTable = indexer.index->addTable("loc", locColumns);

    CXCursor tuCursor = clang_getTranslationUnitCursor(tu);
    clang_visitChildren(tuCursor, &TUIndexer::visitor, &indexer);
    indexer.index->setReadOnly();

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(cxindex);

    for (char *arg : args) {
        free(arg);
    }

    return indexer.index;
}

static std::vector<std::string> readJsonStringList(const Json::Value &json)
{
    std::vector<std::string> result;
    for (const Json::Value &element : json) {
        result.push_back(element.asString());
    }
    return result;
}

void readSourcesJson(const Json::Value &json, indexdb::Index *index)
{
    std::vector<std::string> paths;
    std::vector<QFuture<indexdb::Index*> > fileIndices;

    for (Json::ValueIterator it = json.begin(), itEnd = json.end();
            it != itEnd; ++it) {
        Json::Value &sourceJson = *it;
        SourceFileInfo sfi;
        sfi.path = sourceJson["file"].asString();
        sfi.defines = readJsonStringList(sourceJson["defines"]);
        sfi.includes = readJsonStringList(sourceJson["includes"]);
        sfi.extraArgs = readJsonStringList(sourceJson["extraArgs"]);
        paths.push_back(sfi.path);
        fileIndices.push_back(std::move(QtConcurrent::run(indexSourceFile, sfi)));
    }

    for (size_t i = 0; i < paths.size(); ++i) {
        std::cout << paths[i] << std::endl;
        indexdb::Index *fileIndex = fileIndices[i].result();
        index->merge(*fileIndex);
        delete fileIndex;
    }
}

void readSourcesJson(const std::string &filename, indexdb::Index *index)
{
    std::ifstream f(filename.c_str());
    Json::Reader r;
    Json::Value rootJson;
    r.parse(f, rootJson);
    readSourcesJson(rootJson, index);
}

int main(int argc, char *argv[])
{
    indexdb::Index *index;

    if (1) {
        index = new indexdb::Index;
        readSourcesJson(std::string("btrace.sources"), index);
        index->setReadOnly();
        index->dump();
        index->save("index");

        delete index;
    } else if (1) {
        index = new indexdb::Index("index");
        index->dump();

        indexdb::Table *table = index->table("ref");
        indexdb::Row row(table->columnCount());
        indexdb::HashSet<char> *usr = index->stringTable("usr");
        indexdb::HashSet<char> *path = index->stringTable("path");
        indexdb::HashSet<char> *kind = index->stringTable("kind");

        indexdb::ID symbol = usr->id("c:IdentifierResolver.cpp@10052@N@clang@C@IdentifierResolver@F@tryAddTopLevelDecl#*$@N@clang@C@NamedDecl#$@N@clang@C@DeclarationName#@IDI");
        indexdb::Row r(1);
        r[0] = symbol;
        auto it = table->lowerBound(r);
        while (true) {
            it.value(r);
            if (r[0] != symbol)
                break;
            std::pair<const char*, uint32_t> pathData = path->data(r[1]);
            std::pair<const char*, uint32_t> kindData = kind->data(r[4]);
            std::cout << std::string(pathData.first, pathData.second) << ":" << r[2] << ":" << r[3] << " -- " << std::string(kindData.first, kindData.second) << std::endl;
            ++it;
        }

        delete index;
    }

    return 0;
}
