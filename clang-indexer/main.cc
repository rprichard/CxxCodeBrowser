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
};

static inline const char *nullToBlank(const char *string)
{
    return (string != NULL) ? string : "";
}

static uint32_t readVleUInt32(const char *&buffer)
{
    uint32_t result = 0;
    unsigned char nibble;
    int bit = 0;
    do {
        nibble = static_cast<unsigned char>(*(buffer++));
        result |= (nibble & 0x7F) << bit;
        bit += 7;
    } while ((nibble & 0x80) == 0x80);
    return result;
}

// This encoding of a uint32 is guaranteed not to contain NUL bytes unless the
// value is zero.
static void writeVleUInt32(char *&buffer, uint32_t value)
{
    do {
        unsigned char nibble = value & 0x7F;
        value >>= 7;
        if (value != 0)
            nibble |= 0x80;
        *buffer++ = nibble;
    } while (value != 0);
}

// TODO: Try making src const.
void mergeIndex(indexdb::Index &dest, indexdb::Index &src)
{
    const char *tables[] = { "usr", "path", "kind" };
    std::vector<indexdb::ID> idMap[3];

    for (int i = 0; i < 3; ++i) {
        indexdb::HashSet<char> *destTable = dest.stringTable(tables[i]);
        indexdb::HashSet<char> *srcTable = src.stringTable(tables[i]);
        idMap[i].resize(srcTable->size());
        for (indexdb::ID id = 0; id < srcTable->size(); ++id) {
            std::pair<const char *, uint32_t> data = srcTable->data(id);
            idMap[i][id] = destTable->id(data.first, data.second, srcTable->hash(id));
        }
    }

    for (auto it = src.begin(); it != src.end(); ++it) {
        const char *string = *it;
        if (string[0] == 'R') {
            string++;
            uint32_t usrID = readVleUInt32(string) - 1;
            uint32_t fileNameID = readVleUInt32(string) - 1;
            uint32_t line = readVleUInt32(string) - 1;
            uint32_t column = readVleUInt32(string) - 1;
            uint32_t kindID = readVleUInt32(string) - 1;
            assert(*string == '\0');

            assert(usrID < idMap[0].size());
            assert(fileNameID < idMap[1].size());
            assert(kindID < idMap[2].size());
            usrID = idMap[0][usrID] + 1;
            fileNameID = idMap[1][fileNameID] + 1;
            kindID = idMap[2][kindID] + 1;

            char buffer[256], *pbuffer = buffer;
            *pbuffer++ = 'R';
            writeVleUInt32(pbuffer, usrID + 1);
            writeVleUInt32(pbuffer, fileNameID + 1);
            writeVleUInt32(pbuffer, line + 1);
            writeVleUInt32(pbuffer, column + 1);
            writeVleUInt32(pbuffer, kindID + 1);
            *pbuffer++ = '\0';
            dest.addString(buffer);
        } else if (string[0] == 'L') {
            string++;
            uint32_t fileNameID = readVleUInt32(string) - 1;
            uint32_t line = readVleUInt32(string) - 1;
            uint32_t column = readVleUInt32(string) - 1;
            uint32_t usrID = readVleUInt32(string) - 1;
            assert(*string == '\0');

            assert(usrID < idMap[0].size());
            assert(fileNameID < idMap[1].size());
            usrID = idMap[0][usrID] + 1;
            fileNameID = idMap[1][fileNameID] + 1;

            char buffer[256], *pbuffer = buffer;
            *pbuffer++ = 'L';
            writeVleUInt32(pbuffer, fileNameID + 1);
            writeVleUInt32(pbuffer, line + 1);
            writeVleUInt32(pbuffer, column + 1);
            writeVleUInt32(pbuffer, usrID + 1);
            *pbuffer++ = '\0';
            dest.addString(buffer);
        }
    }
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

            char buffer[256], *pbuffer;

            pbuffer = buffer;
            *pbuffer++ = 'R';
            writeVleUInt32(pbuffer, usrID + 1);
            writeVleUInt32(pbuffer, fileNameID + 1);
            writeVleUInt32(pbuffer, line + 1);
            writeVleUInt32(pbuffer, column + 1);
            writeVleUInt32(pbuffer, kindID + 1);
            *pbuffer++ = '\0';
            index->addString(buffer);

            pbuffer = buffer;
            *pbuffer++ = 'L';
            writeVleUInt32(pbuffer, fileNameID + 1);
            writeVleUInt32(pbuffer, line + 1);
            writeVleUInt32(pbuffer, column + 1);
            writeVleUInt32(pbuffer, usrID + 1);
            *pbuffer++ = '\0';
            index->addString(buffer);
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

    indexer.pathStringTable = indexer.index->stringTable("path");
    indexer.kindStringTable = indexer.index->stringTable("kind");
    indexer.usrStringTable = indexer.index->stringTable("usr");

    CXCursor tuCursor = clang_getTranslationUnitCursor(tu);
    clang_visitChildren(tuCursor, &TUIndexer::visitor, &indexer);
    indexer.index->setStringSetState(indexdb::Index::StringSetArray);

    {
        std::lock_guard<std::mutex> lock(*mutex);
        indexer.index->setStringSetState(indexdb::Index::StringSetArray);
        mergeIndex(*index, *indexer.index);
        //index->merge(*indexer.index);
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
        index->setStringSetState(indexdb::Index::StringSetArray);
        index->dump();
        index->save("index");

        /*
        for (auto it = index->begin(); it != index->end(); ++it) {
            std::cout << *it << std::endl;
        }
        */

        delete index;
    } else if (1) {
        index = new indexdb::Index("index");
        index->dump();
        delete index;
    }

    return 0;
}
