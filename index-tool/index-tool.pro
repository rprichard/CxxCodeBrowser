QT -= core gui

TARGET = index-tool
TEMPLATE = app

SOURCES += \
    main.cc

DEPENDENCY_LIBS = \
    ../libindexdb/libindexdb.a \
    ../third_party/libsnappy/libsnappy.a \
    ../third_party/libMurmurHash3/libMurmurHash3.a \
    ../third_party/libsha2/libsha2.a
include(../dependency_libs.pri)

QMAKE_CXXFLAGS += -std=c++0x
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
