#include <QtDebug>
#include <QtCore>
#include <QString>
#include <QStringList>
#include <fstream>
#include <vector>
#include <mutex>
#include <json/reader.h>
#include <string.h>
#include <assert.h>
#include <clang-c/Index.h>
#include "IndexDb.h"


// TODO: Is there a fundamental reason we can't have a tuple inside another
// tuple?  i.e. Maybe we could allow arbitrary trees on either side of the
// id->id mapping.  Need to consider the effect on tuple hash codes.  (Will
// they be stable across merging? I think they could be.)  Need to consider
// how/whether ID remapping is affected during merging -- I think it isn't
// affected because we'd remap earlier IDs first -- and a tuple will always
// have a greater ID than its contained tuples.


struct TUIndexer {
    CXChildVisitResult visitor(
            CXCursor cursor,
            CXCursor parent);
    static CXChildVisitResult visitor(
            CXCursor cursor,
            CXCursor parent,
            CXClientData data);

    indexdb::Index *index;
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

            indexdb::ID usrID = index->stringID(usrCStr);
            indexdb::ID kindID = index->stringID(kindCStr);
            indexdb::ID fileNameID = index->stringID(fileNameCStr);

            char tmp[8];
            for (int i = 0; i < 8; ++i) {
                tmp[7 - i] = "0123456789abcdef"[(line >> (i * 4)) & 15];
            }
            indexdb::ID lineID = index->stringID(tmp, 8);
            for (int i = 0; i < 8; ++i) {
                tmp[7 - i] = "0123456789abcdef"[(column >> (i * 4)) & 15];
            }
            indexdb::ID columnID = index->stringID(tmp, 8);

            indexdb::ID location[3] = { fileNameID, lineID, columnID };
            indexdb::ID locationID = index->tupleID(location, 3);
            indexdb::ID ref[4] = { fileNameID, lineID, columnID, kindID };
            indexdb::ID refID = index->tupleID(ref, 4);

            index->addPair(usrID, refID);
            index->addPair(locationID, usrID);
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

void indexSourceFile(
        SourceFileInfo sfi,
        indexdb::Index *index,
        std::mutex *mutex)
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
    indexer.index = new indexdb::Index; // index;
    CXCursor tuCursor = clang_getTranslationUnitCursor(tu);
    clang_visitChildren(tuCursor, &TUIndexer::visitor, &indexer);
    indexer.index->setPairState(indexdb::Index::PairArray);

    {
        std::lock_guard<std::mutex> lock(*mutex);
        index->merge(*indexer.index);
    }

    delete indexer.index;

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(cxindex);

    for (char *arg : args) {
        free(arg);
    }
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
    QList<QFuture<void> > futures;

    std::mutex mutex;

    for (Json::ValueIterator it = json.begin(), itEnd = json.end();
            it != itEnd; ++it) {
        Json::Value &sourceJson = *it;
        SourceFileInfo sfi;
        sfi.path = sourceJson["file"].asString();
        sfi.defines = readJsonStringList(sourceJson["defines"]);
        sfi.includes = readJsonStringList(sourceJson["includes"]);
        sfi.extraArgs = readJsonStringList(sourceJson["extraArgs"]);

        std::cerr << sfi.path << std::endl;
        //indexSourceFile(sfi, index, &mutex);
        futures << QtConcurrent::run(indexSourceFile, sfi, index, &mutex);
        //futures.last().waitForFinished(); // TODO: remove this
    }

    // TODO: Consider requiring that files be merged in a deterministic order
    // so that the final merged index is also deterministic.

    foreach (QFuture<void> future, futures) {
        future.waitForFinished();
    }
}

void readSourcesJson(const QString &filename, indexdb::Index *index)
{
    std::ifstream f(filename.toStdString().c_str());
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
        readSourcesJson(QString("btrace.sources"), index);
        index->setPairState(indexdb::Index::PairArray);
        index->dump();
        index->save("index");
        delete index;
    } else if (1) {
        index = new indexdb::Index("index");
        index->dump();
        delete index;
    }


    return 0;
}
