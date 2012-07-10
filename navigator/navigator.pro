QT += core gui

TARGET = navigator
TEMPLATE = app

SOURCES += \
    SourcesJsonReader.cc \
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
    SourceWidget.cc \
    ReportRefList.cc \
    ReportCSources.cc \
    TreeReport.cc \
    TreeReportWindow.cc

HEADERS += \
    SourcesJsonReader.h \
    Misc.h \
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
    SourceWidget.h \
    ReportRefList.h \
    ReportCSources.h \
    TreeReport.h \
    TreeReportWindow.h

FORMS += \
    MainWindow.ui \
    TreeReportWindow.ui

target.path = /
INSTALLS += target

include(../clang.pri)

CONFIG += link_pkgconfig
PKGCONFIG += jsoncpp

QMAKE_CXXFLAGS += -Wall -Wno-unused-parameter
