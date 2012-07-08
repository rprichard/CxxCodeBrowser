QT += core gui

TARGET = navigator
TEMPLATE = app

SOURCES += \
    SourcesJsonReader.cc \
    NavTableWindow.cc \
    TableSupplierSourceList.cc \
    main.cc \
    NavMainWindow.cc \
    NavCommandWidget.cc \
    Misc.cc \
    NavSourceWidget.cc \
    Project.cc \
    CSource.cc \
    FileManager.cc \
    File.cc \
    SymbolTable.cc \
    Symbol.cc \
    Ref.cc \
    SourceIndexer.cc \
    TableSupplierRefList.cc

HEADERS += \
    SourcesJsonReader.h \
    Misc.h \
    NavTableWindow.h \
    TableSupplier.h \
    TableSupplierSourceList.h \
    NavMainWindow.h \
    NavCommandWidget.h \
    NavSourceWidget.h \
    Project.h \
    CSource.h \
    FileManager.h \
    File.h \
    SymbolTable.h \
    Symbol.h \
    Ref.h \
    SourceIndexer.h \
    TableSupplierRefList.h

FORMS += \
    NavTableWindow.ui \
    NavMainWindow.ui

target.path = /
INSTALLS += target

include(../clang.pri)

CONFIG += link_pkgconfig
PKGCONFIG += jsoncpp

QMAKE_CXXFLAGS += -Wall -Wno-unused-parameter