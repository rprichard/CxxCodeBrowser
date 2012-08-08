QT += core

TARGET = clang-index-test
TEMPLATE = app

SOURCES += test.cc

LIBS += -lclang

target.path = /
INSTALLS += target
