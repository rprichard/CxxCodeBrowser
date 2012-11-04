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

PRE_TARGETDEPS += $${OUT_PWD}/../libindexdb/libindexdb.a
LIBS           += $${OUT_PWD}/../libindexdb/libindexdb.a

INCLUDEPATH    += ../third_party/libsnappy
PRE_TARGETDEPS += $${OUT_PWD}/../third_party/libsnappy/libsnappy.a
LIBS           += $${OUT_PWD}/../third_party/libsnappy/libsnappy.a

INCLUDEPATH    += ../third_party/libMurmurHash3
PRE_TARGETDEPS += $${OUT_PWD}/../third_party/libMurmurHash3/libMurmurHash3.a
LIBS           += $${OUT_PWD}/../third_party/libMurmurHash3/libMurmurHash3.a

INCLUDEPATH    += ../third_party/libsha2
PRE_TARGETDEPS += $${OUT_PWD}/../third_party/libsha2/libsha2.a
LIBS           += $${OUT_PWD}/../third_party/libsha2/libsha2.a

INCLUDEPATH    += ../third_party/libre2
PRE_TARGETDEPS += $${OUT_PWD}/../third_party/libre2/libre2.a
LIBS           += $${OUT_PWD}/../third_party/libre2/libre2.a

target.path = /
INSTALLS += target

QMAKE_CXXFLAGS += -std=c++0x
QMAKE_CXXFLAGS_WARN_ON += -Wall -Wno-unused-parameter
