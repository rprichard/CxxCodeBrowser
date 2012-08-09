QT += core

TARGET = clang-indexer
TEMPLATE = app

SOURCES += \
    main.cc

LIBS += -lclang

CONFIG += link_pkgconfig
PKGCONFIG += jsoncpp

target.path = /
INSTALLS += target

QMAKE_CXXFLAGS += -Wall -Wno-unused-parameter
