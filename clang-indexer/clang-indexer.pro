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

DEFINES        += JSON_IS_AMALGAMATION
INCLUDEPATH    += ../third_party/libjsoncpp
PRE_TARGETDEPS += $${OUT_PWD}/../third_party/libjsoncpp/libjsoncpp.a
LIBS           += $${OUT_PWD}/../third_party/libjsoncpp/libjsoncpp.a

PRE_TARGETDEPS += $${OUT_PWD}/../libindexdb/libindexdb.a
LIBS           += $${OUT_PWD}/../libindexdb/libindexdb.a

INCLUDEPATH    += ../third_party/libsnappy
PRE_TARGETDEPS += $${OUT_PWD}/../third_party/libsnappy/libsnappy.a
LIBS           += $${OUT_PWD}/../third_party/libsnappy/libsnappy.a

target.path = /
INSTALLS += target

QMAKE_CXXFLAGS += -std=c++0x
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter

include(../clang.pri)
