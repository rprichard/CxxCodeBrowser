include(../config.pri)

QT -= core gui

TARGET = sw-linux-elfvinfo
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle

SOURCES += \
    evi_reader.c \
    elfvinfo.c

QMAKE_LIBS += -ldl
include(../enable-c99.pri)

target.path = $$LIBEXEC_DIR
INSTALLS += target
