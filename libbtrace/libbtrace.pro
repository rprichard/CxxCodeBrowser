QT -= core gui
CONFIG += dll

# HACK: Make qmake use the C driver rather than the C++ driver.
QMAKE_LINK = $$QMAKE_LINK_C

TARGET = btrace
TEMPLATE = lib

SOURCES += \
    libbtrace.c

DEFINES += _GNU_SOURCE

LIBS += -ldl

QMAKE_CFLAGS += -std=c99

target.path = /
INSTALLS += target
