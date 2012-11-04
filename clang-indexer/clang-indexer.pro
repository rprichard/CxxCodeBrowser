QT += core

TARGET = clang-indexer
TEMPLATE = app

SOURCES += \
    ASTIndexer.cc \
    DaemonPool.cc \
    IndexBuilder.cc \
    IndexerContext.cc \
    IndexerPPCallbacks.cc \
    NameGenerator.cc \
    Process.cc \
    TUIndexer.cc \
    Util.cc \
    main.cc

HEADERS += \
    ASTIndexer.h \
    DaemonPool.h \
    IndexBuilder.h \
    IndexerContext.h \
    IndexerPPCallbacks.h \
    Location.h \
    NameGenerator.h \
    Process.h \
    Switcher.h \
    TUIndexer.h \
    Util.h

DEFINES += JSON_IS_AMALGAMATION
INCLUDEPATH += ../third_party/libjsoncpp
DEPENDENCY_LIBS = \
    ../libindexdb/libindexdb.a \
    ../third_party/libjsoncpp/libjsoncpp.a \
    ../third_party/libsnappy/libsnappy.a \
    ../third_party/libMurmurHash3/libMurmurHash3.a \
    ../third_party/libsha2/libsha2.a
include(../dependency_libs.pri)

target.path = /
INSTALLS += target

QMAKE_CXXFLAGS += -std=c++0x
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter

include(../clang.pri)
