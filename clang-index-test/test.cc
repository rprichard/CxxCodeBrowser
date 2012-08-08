#include <clang-c/Index.h>
#include <assert.h>
#include <vector>

std::vector<CXCursor> visitStack;

CXChildVisitResult visitor(
        CXCursor cursor,
        CXCursor parent,
        CXClientData data)
{
    {
        // Update the visitStack.
        size_t parentIndex = visitStack.size();
        for (size_t i = 0; i < visitStack.size(); ++i) {
            if (clang_equalCursors(visitStack[i], parent)) {
                parentIndex = i;
                break;
            }
        }
        assert(parentIndex < visitStack.size());
        visitStack.resize(parentIndex + 1);
        visitStack.push_back(cursor);
    }

    CXFile file;
    unsigned int line;
    unsigned int column;
    unsigned int offset;

    for (size_t i = 2; i < visitStack.size(); ++i) {
        printf("   ");
    }

    clang_getInstantiationLocation(
            clang_getCursorLocation(cursor),
                &file, &line, &column, &offset);
    printf("[%p:%s:%d:%d]",
           file,
           clang_getCString(clang_getFileName(file)),
           line, column);

#if 0
    clang_getInstantiationLocation(
                clang_getRangeStart(clang_getCursorExtent(cursor)),
                &file, &line, &column, &offset);
    printf("[%s:%d:%d]",
            clang_getCString(clang_getFileName(file)),
            line, column);

    clang_getInstantiationLocation(
                clang_getRangeEnd(clang_getCursorExtent(cursor)),
                &file, &line, &column, &offset);
    printf("[%s:%d:%d]",
            clang_getCString(clang_getFileName(file)),
            line, column);
#endif


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

    return CXChildVisit_Recurse;
}

int main()
{
    const char *args[] = {
        "-DFOO=BAR",
    };

    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit tu = clang_parseTranslationUnit(
                index, "/home/rprichard/project/test.cc",
                args, sizeof(args) / sizeof(args[0]),
                NULL, 0,
                CXTranslationUnit_DetailedPreprocessingRecord // CXTranslationUnit_None
                );

    CXCursor tuCursor = clang_getTranslationUnitCursor(tu);
    visitStack.push_back(tuCursor);
    clang_visitChildren(tuCursor, visitor, NULL);

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);

    return 0;
}

