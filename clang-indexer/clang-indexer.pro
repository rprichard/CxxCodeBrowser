QT += core

TARGET = clang-indexer
TEMPLATE = app

SOURCES += \
    main.cc \
    ASTIndexer.cc \
    Location.cc \
    IndexerPPCallbacks.cc \
    TUIndexer.cc \
    IndexBuilder.cc \
    IndexerContext.cc \
    NameGenerator.cc \
    SubprocessManager.cc \
    Util.cc

CONFIG += link_pkgconfig
PKGCONFIG += jsoncpp

PRE_TARGETDEPS += $${OUT_PWD}/../libindexdb/libindexdb.a
LIBS +=           $${OUT_PWD}/../libindexdb/libindexdb.a

target.path = /
INSTALLS += target

QMAKE_CXXFLAGS += -Wall -Wno-unused-parameter -std=c++0x

HEADERS += \
    Switcher.h \
    ASTIndexer.h \
    Location.h \
    IndexerPPCallbacks.h \
    TUIndexer.h \
    IndexBuilder.h \
    IndexerContext.h \
    NameGenerator.h \
    SubprocessManager.h \
    Util.h

include(../clang.pri)
