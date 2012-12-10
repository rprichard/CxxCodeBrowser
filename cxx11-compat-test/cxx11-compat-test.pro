QT -= core gui

TARGET = cxx11-compat-test
TEMPLATE = app
CONFIG += console

SOURCES += \
    main.cc

QMAKE_CXXFLAGS += -std=c++0x
