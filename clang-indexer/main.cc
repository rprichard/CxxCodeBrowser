#include <QtCore>
#include <QtDebug>
#include <cassert>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

#include <json/reader.h>
#include <clang-c/Index.h>

#include "../libindexdb/IndexDb.h"
#include "IndexBuilder.h"
#include "TUIndexer.h"

// The Clang driver uses this driver path to locate its built-in include files
// which are at ../lib/clang/<VERSION>/include from the bin directory.
#define kDriverPath "/home/rprichard/llvm-install/bin/clang"

#if 0
struct TUIndexer {
    CXChildVisitResult visitor(
            CXCursor cursor,
            CXCursor parent);
    static CXChildVisitResult visitor(
            CXCursor cursor,
            CXCursor parent,
            CXClientData data);

    indexdb::Index *index;

    indexdb::StringTable *pathStringTable;
    indexdb::StringTable *kindStringTable;
    indexdb::StringTable *usrStringTable;

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

            indexdb::ID usrID = usrStringTable->insert(usrCStr);
            indexdb::ID fileNameID = pathStringTable->insert(fileNameCStr);
            indexdb::ID kindID = kindStringTable->insert(kindCStr);

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
#endif

struct SourceFileInfo {
    std::string path;
    std::vector<std::string> defines;
    std::vector<std::string> includes;
    std::vector<std::string> extraArgs;
};

static bool stringEndsWith(const std::string &text, const std::string &ending)
{
    return text.size() >= ending.size() &&
            text.rfind(ending) == text.size() - ending.size();
}

indexdb::Index *indexSourceFile(SourceFileInfo sfi)
{
    std::vector<std::string> argv;

    bool isCXX = stringEndsWith(sfi.path, ".cc") ||
            stringEndsWith(sfi.path, ".cpp") ||
            stringEndsWith(sfi.path, ".cxx") ||
            stringEndsWith(sfi.path, ".c++") ||
            stringEndsWith(sfi.path, ".C");

    argv.push_back(isCXX ? kDriverPath"++" : kDriverPath);
    argv.push_back("-c");
    argv.push_back(sfi.path);
    for (auto define : sfi.defines) {
        argv.push_back("-D" + define);
    }
    for (auto include : sfi.includes) {
        argv.push_back("-I" + include);
    }
    for (auto arg : sfi.extraArgs) {
        argv.push_back(arg);
    }

    indexdb::Index *index = indexer::newIndex();
    indexer::indexTranslationUnit(argv, index);
    index->setReadOnly();
    return index;

#if 0
    CXIndex cxindex = clang_createIndex(0, 0);
    CXTranslationUnit tu = clang_parseTranslationUnit(
                cxindex,
                sfi.path.c_str(),
                args.data(), args.size(),
                NULL, 0,
                CXTranslationUnit_DetailedPreprocessingRecord // CXTranslationUnit_None
                );
    if (!tu) {
        std::stringstream ss;
        ss << "Error parsing translation unit: " << sfi.path;
        for (size_t i = 0; i < args.size(); ++i) {
            ss << " " << args[i];
        }
        ss << std::endl;
        std::cerr << ss.str();
        return newIndex();
    }

    assert(tu);

    TUIndexer indexer;
    indexer.index = newIndex();

    indexer.pathStringTable = indexer.index->stringTable("path");
    indexer.kindStringTable = indexer.index->stringTable("kind");
    indexer.usrStringTable = indexer.index->stringTable("usr");
    indexer.refTable = indexer.index->table("ref");
    indexer.locTable = indexer.index->table("loc");

    CXCursor tuCursor = clang_getTranslationUnitCursor(tu);
    clang_visitChildren(tuCursor, &TUIndexer::visitor, &indexer);
    indexer.index->setReadOnly();

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(cxindex);

    for (char *arg : argv) {
        free(arg);
    }
#endif
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
    indexdb::Index *index = new indexdb::Index;
    readSourcesJson(std::string("btrace.sources"), index);
    index->setReadOnly();
    index->save("index");
    return 0;
}
