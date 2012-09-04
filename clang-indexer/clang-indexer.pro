QT += core

TARGET = clang-indexer
TEMPLATE = app

SOURCES += \
    main.cc

LIBS += -lclang
CONFIG += link_pkgconfig
PKGCONFIG += jsoncpp

PRE_TARGETDEPS += $${OUT_PWD}/../libindexdb/libindexdb.a
LIBS +=           $${OUT_PWD}/../libindexdb/libindexdb.a

target.path = /
INSTALLS += target

QMAKE_CXXFLAGS += -Wall -Wno-unused-parameter -std=c++0x

HEADERS += \
    Switcher.h
