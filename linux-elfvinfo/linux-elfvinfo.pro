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

target.path = $$LIBEXEC_DIR
INSTALLS += target

include(../enable-linux-bare-c99.pri)
