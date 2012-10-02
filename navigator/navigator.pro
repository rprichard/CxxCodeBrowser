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
    GotoWindow.cc \
    TextWidthCalculator.cc \
    History.cc \
    Folder.cc \
    FolderItem.cc \
    FolderWidget.cc

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
    GotoWindow.h \
    TextWidthCalculator.h \
    History.h \
    Folder.h \
    FolderItem.h \
    FolderWidget.h

FORMS += \
    MainWindow.ui \
    TreeReportWindow.ui

PRE_TARGETDEPS += $${OUT_PWD}/../libindexdb/libindexdb.a
LIBS +=           $${OUT_PWD}/../libindexdb/libindexdb.a

PRE_TARGETDEPS += $${OUT_PWD}/../libre2/libre2.a
LIBS +=           $${OUT_PWD}/../libre2/libre2.a
INCLUDEPATH += ../libre2

LIBS += -lsnappy

target.path = /
INSTALLS += target

QMAKE_CXXFLAGS += -std=c++0x
QMAKE_CXXFLAGS_WARN_ON += -Wall -Wno-unused-parameter
