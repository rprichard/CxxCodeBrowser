QT -= core gui

TARGET = cxx11-compat-test
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle

SOURCES += \
    main.cc

include(../enable-cxx11.pri)
