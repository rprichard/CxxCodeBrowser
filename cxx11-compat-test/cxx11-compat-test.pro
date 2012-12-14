QT -= core gui

TARGET = cxx11-compat-test
TEMPLATE = app
CONFIG += console

SOURCES += \
    main.cc

include(../enable-cxx11.pri)
