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
    USRGenerator.cc \
    IndexerContext.cc

#LIBS += -lclang
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
    USRGenerator.h \
    IndexerContext.h

include(../clang.pri)
