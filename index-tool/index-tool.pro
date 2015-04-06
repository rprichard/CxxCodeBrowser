include(../config.pri)

QT -= core gui

TARGET = sw-index-tool
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle

SOURCES += \
    main.cc

OTHER_FILES += \
    dependencies.cfg

ROOT_DIR = ..
include(../add_dependencies.pri)

target.path = $$BIN_DIR
INSTALLS += target

include(../enable-cxx11.pri)
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
