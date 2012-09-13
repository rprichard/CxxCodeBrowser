QT += core gui

TARGET = navigator
TEMPLATE = app

SOURCES += \
    main.cc \
    MainWindow.cc \
    CommandWidget.cc \
    Misc.cc \
    Project.cc \
    FileManager.cc \
    File.cc \
    Ref.cc \
    SourceWidget.cc \
    ReportFileList.cc \
    ReportRefList.cc \
    TreeReport.cc \
    TreeReportWindow.cc \
    CXXSyntaxHighlighter.cc \
    GotoWindow.cc

HEADERS += \
    Misc.h \
    MainWindow.h \
    CommandWidget.h \
    Project.h \
    FileManager.h \
    File.h \
    Ref.h \
    SourceWidget.h \
    ReportFileList.h \
    ReportRefList.h \
    TreeReport.h \
    TreeReportWindow.h \
    CXXSyntaxHighlighter.h \
    CXXSyntaxHighlighterDirectives.h \
    CXXSyntaxHighlighterKeywords.h \
    GotoWindow.h

FORMS += \
    MainWindow.ui \
    TreeReportWindow.ui

PRE_TARGETDEPS += $${OUT_PWD}/../libindexdb/libindexdb.a
LIBS +=           $${OUT_PWD}/../libindexdb/libindexdb.a

PRE_TARGETDEPS += $${OUT_PWD}/../libre2/libre2.a
LIBS +=           $${OUT_PWD}/../libre2/libre2.a
INCLUDEPATH += ../libre2

target.path = /
INSTALLS += target

QMAKE_CXXFLAGS += -Wall -Wno-unused-parameter -std=c++0x
