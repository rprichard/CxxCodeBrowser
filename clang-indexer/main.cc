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
