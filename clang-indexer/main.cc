#include <QtDebug>
#include <QtCore>
#include <QString>
#include <QStringList>
#include <fstream>
#include <vector>
#include <json/reader.h>
#include <string.h>
#include <assert.h>
#include <clang-c/Index.h>



struct TUIndexer {
    std::vector<CXCursor> visitStack;
    CXChildVisitResult visitor(
            CXCursor cursor,
            CXCursor parent);
    static CXChildVisitResult visitor(
            CXCursor cursor,
            CXCursor parent,
            CXClientData data);
};

CXChildVisitResult TUIndexer::visitor(
        CXCursor cursor,
        CXCursor parent)
{
    bool success = true;

    if (1) { // TODO: Why does this assert fail when indexing clang?
        // Update the visitStack.
        size_t parentIndex = visitStack.size();
        for (size_t i = 0; i < visitStack.size(); ++i) {
            if (clang_equalCursors(visitStack[i], parent)) {
                parentIndex = i;
                break;
            }
        }
        success = parentIndex < visitStack.size();
        visitStack.resize(parentIndex + 1);
        visitStack.push_back(cursor);
    }

    CXFile file;
    unsigned int line;
    unsigned int column;
    unsigned int offset;

#if 1
    for (size_t i = 0; i < visitStack.size(); ++i) {
        printf("   ");
    }

    clang_getInstantiationLocation(
            clang_getCursorLocation(cursor),
                &file, &line, &column, &offset);
    printf("[%p:%s:%d:%d]",
           file,
           clang_getCString(clang_getFileName(file)),
           line, column);

    printf(" %s:%s",
           clang_getCString(clang_getCursorKindSpelling(clang_getCursorKind(cursor))),
           clang_getCString(clang_getCursorUSR(cursor)));

    { // if (clang_isReference(clang_getCursorKind(cursor))) {
        CXCursor cursorRef = clang_getCursorReferenced(cursor);
        if (!clang_Cursor_isNull(cursorRef)) {
            printf(" [ref:%s]", clang_getCString(clang_getCursorUSR(cursorRef)));
        }
    }

    printf(" \"%s\"",
           clang_getCString(clang_getCursorDisplayName(cursor)));
    printf("\n");
#endif

    assert(success);

    return CXChildVisit_Recurse;
}

CXChildVisitResult TUIndexer::visitor(
        CXCursor cursor,
        CXCursor parent,
        CXClientData data)
{
    return static_cast<TUIndexer*>(data)->visitor(cursor, parent);
}

void indexSourceFile(
        const QString &path,
        const QStringList &defines,
        const QStringList &includes,
        const QStringList &extraArgs)
{
    qDebug() << path;

    std::vector<char*> args;
    foreach (QString define, defines) {
        QString arg = "-D" + define;
        args.push_back(strdup(arg.toStdString().c_str()));
    }
    foreach (QString include, includes) {
        QString arg = "-I" + include;
        args.push_back(strdup(arg.toStdString().c_str()));
    }
    foreach (QString arg, extraArgs) {
        args.push_back(strdup(arg.toStdString().c_str()));
    }

    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit tu = clang_parseTranslationUnit(
                index,
                path.toStdString().c_str(),
                args.data(), args.size(),
                NULL, 0,
                CXTranslationUnit_DetailedPreprocessingRecord // CXTranslationUnit_None
                );

    TUIndexer indexer;
    CXCursor tuCursor = clang_getTranslationUnitCursor(tu);
    indexer.visitStack.push_back(tuCursor);
    clang_visitChildren(tuCursor, &TUIndexer::visitor, &indexer);

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);

    foreach (char *arg, args) {
        free(arg);
    }
}

static QStringList readJsonStringList(const Json::Value &json)
{
    QStringList result;
    for (Json::ValueIterator it = json.begin(), itEnd = json.end();
            it != itEnd; ++it) {
        Json::Value &element = *it;
        result << QString::fromStdString(element.asString());
    }
    return result;
}

void readSourcesJson(const Json::Value &json)
{
    QList<QFuture<void> > futures;

    for (Json::ValueIterator it = json.begin(), itEnd = json.end();
            it != itEnd; ++it) {
        Json::Value &sourceJson = *it;
        QString path = QString::fromStdString(sourceJson["file"].asString());
        QStringList defines = readJsonStringList(sourceJson["defines"]);
        QStringList includes = readJsonStringList(sourceJson["includes"]);
        QStringList extraArgs = readJsonStringList(sourceJson["extraArgs"]);

        futures << QtConcurrent::run(indexSourceFile, path, defines, includes, extraArgs);
        futures.last().waitForFinished(); // TODO: remove this
    }

    foreach (QFuture<void> future, futures) {
        future.waitForFinished();
    }
}

void readSourcesJson(const QString &filename)
{
    std::ifstream f(filename.toStdString().c_str());
    Json::Reader r;
    Json::Value rootJson;
    r.parse(f, rootJson);
    readSourcesJson(rootJson);
}

int main(int argc, char *argv[])
{
    readSourcesJson(QString("btrace.sources"));
    return 0;
}
