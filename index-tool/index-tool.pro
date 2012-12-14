include(../config.pri)

QT -= core gui

TARGET = sw-index-tool
TEMPLATE = app
CONFIG += console

SOURCES += \
    main.cc

OTHER_FILES += \
    dependencies.cfg

ROOT_DIR = ..
include(../add_dependencies.pri)

target.path = $$BINDIR
INSTALLS += target

include(../enable-cxx11.pri)
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
