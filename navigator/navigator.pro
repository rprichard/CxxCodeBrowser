QT += core gui

TARGET = navigator
TEMPLATE = app

SOURCES += \
    Program.cc \
    Source.cc \
    SourcesJsonReader.cc \
    NavTableWindow.cc \
    TableSupplierSourceList.cc \
    main.cc \
    NavMainWindow.cc \
    NavCommandWidget.cc \
    Misc.cc \
    NavSourceWidget.cc

HEADERS += \
    Program.h \
    Source.h \
    SourcesJsonReader.h \
    Misc.h \
    NavTableWindow.h \
    TableSupplier.h \
    TableSupplierSourceList.h \
    NavMainWindow.h \
    NavCommandWidget.h \
    NavSourceWidget.h

FORMS += \
    NavTableWindow.ui \
    NavMainWindow.ui

target.path = /
INSTALLS += target

include(../clang.pri)

CONFIG += link_pkgconfig
PKGCONFIG += jsoncpp

QMAKE_CXXFLAGS = -Wall -Wno-unused-parameter
