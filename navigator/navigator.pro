QT += core gui

TARGET = navigator
TEMPLATE = app

SOURCES += \
    SourcesJsonReader.cc \
    TableSupplierSourceList.cc \
    main.cc \
    MainWindow.cc \
    CommandWidget.cc \
    Misc.cc \
    Project.cc \
    CSource.cc \
    FileManager.cc \
    File.cc \
    SymbolTable.cc \
    Symbol.cc \
    Ref.cc \
    SourceIndexer.cc \
    TableSupplierRefList.cc \
    SourceWidget.cc \
    TableWindow.cc

HEADERS += \
    SourcesJsonReader.h \
    Misc.h \
    TableSupplier.h \
    TableSupplierSourceList.h \
    MainWindow.h \
    CommandWidget.h \
    Project.h \
    CSource.h \
    FileManager.h \
    File.h \
    SymbolTable.h \
    Symbol.h \
    Ref.h \
    SourceIndexer.h \
    TableSupplierRefList.h \
    SourceWidget.h \
    TableWindow.h

FORMS += \
    MainWindow.ui \
    TableWindow.ui

target.path = /
INSTALLS += target

include(../clang.pri)

CONFIG += link_pkgconfig
PKGCONFIG += jsoncpp

QMAKE_CXXFLAGS += -Wall -Wno-unused-parameter
