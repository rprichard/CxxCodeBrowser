include(./config.pri)

TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    third_party \
    libindexdb \
    clang-indexer \
    index-tool \
    navigator

# HACK: Stop qmake from attempting to strip the scripts.
QMAKE_STRIP = /bin/echo

# Unix btrace.
linux-*|freebsd-*|darwin-*|macx-* {
    SUBDIRS += btrace
    btrace_script.path = $$BIN_DIR
    btrace_script.files += \
        btrace/sw-btrace \
        btrace/sw-btrace-to-compiledb
    INSTALLS += btrace_script
}

include(./check-clang.pri)

win32:CONFIG(release, debug|release): LIBS += -LE:/LLVM/lib/ -lclangFronten
else:win32:CONFIG(debug, debug|release): LIBS += -LE:/LLVM/lib/ -lclangFrontend
else:unix: LIBS += -LE:/LLVM/lib/ -lclangFronten

INCLUDEPATH += E:/LLVM/include
DEPENDPATH += E:/LLVM/include

win32:CONFIG(release, debug|release): LIBS += -LE:/LLVM/lib/ -lclangSerialization
else:win32:CONFIG(debug, debug|release): LIBS += -LE:/LLVM/lib/ -lclangSerializationd
else:unix: LIBS += -LE:/LLVM/lib/ -lclangSerialization

INCLUDEPATH += E:/LLVM/include
DEPENDPATH += E:/LLVM/include

win32:CONFIG(release, debug|release): LIBS += -LE:/LLVM/lib/ -lclangDriver
else:win32:CONFIG(debug, debug|release): LIBS += -LE:/LLVM/lib/ -lclangDriverd
else:unix: LIBS += -LE:/LLVM/lib/ -lclangDriver

INCLUDEPATH += E:/LLVM/include
DEPENDPATH += E:/LLVM/include

win32:CONFIG(release, debug|release): LIBS += -LE:/LLVM/lib/ -lclangTooling
else:win32:CONFIG(debug, debug|release): LIBS += -LE:/LLVM/lib/ -lclangToolingd
else:unix: LIBS += -LE:/LLVM/lib/ -lclangTooling

INCLUDEPATH += E:/LLVM/include
DEPENDPATH += E:/LLVM/include

win32:CONFIG(release, debug|release): LIBS += -LE:/LLVM/lib/ -lclangParse
else:win32:CONFIG(debug, debug|release): LIBS += -LE:/LLVM/lib/ -lclangParsed
else:unix: LIBS += -LE:/LLVM/lib/ -lclangParse

INCLUDEPATH += E:/LLVM/include
DEPENDPATH += E:/LLVM/include

win32:CONFIG(release, debug|release): LIBS += -LE:/LLVM/lib/ -lclangSema
else:win32:CONFIG(debug, debug|release): LIBS += -LE:/LLVM/lib/ -lclangSemad
else:unix: LIBS += -LE:/LLVM/lib/ -lclangSema

INCLUDEPATH += E:/LLVM/include
DEPENDPATH += E:/LLVM/include

win32:CONFIG(release, debug|release): LIBS += -LE:/LLVM/lib/ -lclangAnalysis
else:win32:CONFIG(debug, debug|release): LIBS += -LE:/LLVM/lib/ -lclangAnalysisd
else:unix: LIBS += -LE:/LLVM/lib/ -lclangAnalysis

INCLUDEPATH += E:/LLVM/include
DEPENDPATH += E:/LLVM/include

win32:CONFIG(release, debug|release): LIBS += -LE:/LLVM/lib/ -lclangEdit
else:win32:CONFIG(debug, debug|release): LIBS += -LE:/LLVM/lib/ -lclangEditd
else:unix: LIBS += -LE:/LLVM/lib/ -lclangEdit

INCLUDEPATH += E:/LLVM/include
DEPENDPATH += E:/LLVM/include

win32:CONFIG(release, debug|release): LIBS += -LE:/LLVM/lib/ -lclangAST
else:win32:CONFIG(debug, debug|release): LIBS += -LE:/LLVM/lib/ -lclangASTd
else:unix: LIBS += -LE:/LLVM/lib/ -lclangAST

INCLUDEPATH += E:/LLVM/include
DEPENDPATH += E:/LLVM/include

win32:CONFIG(release, debug|release): LIBS += -LE:/LLVM/lib/ -lclangLex
else:win32:CONFIG(debug, debug|release): LIBS += -LE:/LLVM/lib/ -lclangLexd
else:unix: LIBS += -LE:/LLVM/lib/ -lclangLex

INCLUDEPATH += E:/LLVM/include
DEPENDPATH += E:/LLVM/include

win32:CONFIG(release, debug|release): LIBS += -LE:/LLVM/lib/ -lclangBasic
else:win32:CONFIG(debug, debug|release): LIBS += -LE:/LLVM/lib/ -lclangBasicd
else:unix: LIBS += -LE:/LLVM/lib/ -lclangBasic

INCLUDEPATH += E:/LLVM/include
DEPENDPATH += E:/LLVM/include
