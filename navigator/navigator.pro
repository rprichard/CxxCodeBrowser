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
    CXXSyntaxHighlighter.cc \
    TextWidthCalculator.cc \
    History.cc \
    Folder.cc \
    FolderItem.cc \
    FolderWidget.cc \
    PlaceholderLineEdit.cc \
    TableReportWindow.cc \
    TableReport.cc \
    TableReportView.cc \
    Regex.cc \
    ReportDefList.cc \
    ReportSymList.cc

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
    CXXSyntaxHighlighter.h \
    CXXSyntaxHighlighterDirectives.h \
    CXXSyntaxHighlighterKeywords.h \
    TextWidthCalculator.h \
    History.h \
    Folder.h \
    FolderItem.h \
    FolderWidget.h \
    StringRef.h \
    PlaceholderLineEdit.h \
    TableReportWindow.h \
    TableReport.h \
    TableReportView.h \
    Regex.h \
    ReportDefList.h \
    ReportSymList.h

FORMS += \
    MainWindow.ui

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
