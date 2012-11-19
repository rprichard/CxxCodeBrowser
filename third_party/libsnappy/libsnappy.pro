# subset of snappy-1.0.5 archive

QT -= core gui
CONFIG += static create_prl

TARGET = snappy
TEMPLATE = lib

DEFINES += HAVE_CONFIG_H

SOURCES += \
    snappy.cc \
    snappy-sinksource.cc \
    snappy-stubs-internal.cc

HEADERS += \
    config.h \
    snappy.h \
    snappy-internal.h \
    snappy-sinksource.h \
    snappy-stubs-internal.h \
    snappy-stubs-public.h

QMAKE_CXXFLAGS_WARN_ON += \
    -Wno-sign-compare \
    -Wno-unused-parameter \
    -Wno-unused-function
