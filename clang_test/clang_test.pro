QT += core

TARGET = clang_test
TEMPLATE = app

SOURCES += test.cc

include(../clang.pri)

target.path = /
INSTALLS += target
