QT += core gui

TARGET = navigator
TEMPLATE = app

SOURCES += \
    CXXSyntaxHighlighter.cc \
    File.cc \
    FileManager.cc \
    FindBar.cc \
    Folder.cc \
    FolderItem.cc \
    FolderWidget.cc \
    History.cc \
    MainWindow.cc \
    Misc.cc \
    PlaceholderLineEdit.cc \
    Project.cc \
    Ref.cc \
    Regex.cc \
    RegexMatchList.cc \
    ReportDefList.cc \
    ReportFileList.cc \
    ReportRefList.cc \
    ReportSymList.cc \
    SourceWidget.cc \
    TableReport.cc \
    TableReportView.cc \
    TableReportWindow.cc \
    TextWidthCalculator.cc \
    main.cc

HEADERS += \
    CXXSyntaxHighlighter.h \
    CXXSyntaxHighlighterDirectives.h \
    CXXSyntaxHighlighterKeywords.h \
    File.h \
    FileManager.h \
    FindBar.h \
    Folder.h \
    FolderItem.h \
    FolderWidget.h \
    History.h \
    MainWindow.h \
    Misc.h \
    PlaceholderLineEdit.h \
    Project.h \
    RandomAccessIterator.h \
    Ref.h \
    Regex.h \
    RegexMatchList.h \
    ReportDefList.h \
    ReportFileList.h \
    ReportRefList.h \
    ReportSymList.h \
    SourceWidget.h \
    StringRef.h \
    TableReport.h \
    TableReportView.h \
    TableReportWindow.h \
    TextWidthCalculator.h

FORMS += \
    MainWindow.ui

INCLUDEPATH += \
    ../third_party/libre2

DEPENDENCY_LIBS = \
    ../libindexdb/libindexdb.a \
    ../third_party/libsnappy/libsnappy.a \
    ../third_party/libMurmurHash3/libMurmurHash3.a \
    ../third_party/libsha2/libsha2.a \
    ../third_party/libre2/libre2.a
include(../dependency_libs.pri)

target.path = /
INSTALLS += target

QMAKE_CXXFLAGS += -std=c++0x
QMAKE_CXXFLAGS_WARN_ON += -Wall -Wno-unused-parameter
