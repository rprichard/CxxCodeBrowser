QT -= core gui

CONFIG += link_prl

TARGET = index-tool
TEMPLATE = app

SOURCES += \
    main.cc

DEPENDENCY_STATIC_LIBS = \
    ../libindexdb
include(../dependency_static_libs.pri)

QMAKE_CXXFLAGS += -std=c++0x
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
