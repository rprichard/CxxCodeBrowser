QT += core

TARGET = clang-indexer
TEMPLATE = app

SOURCES += \
    main.cc \
    IndexDb.cc \
    Buffer.cc \
    MurmurHash3.cpp \
    FileIo.cc

LIBS += -lclang

CONFIG += link_pkgconfig
PKGCONFIG += jsoncpp

target.path = /
INSTALLS += target

QMAKE_CXXFLAGS += -Wall -Wno-unused-parameter -std=c++0x

HEADERS += \
    IndexDb.h \
    HashSet.h \
    Buffer.h \
    HashSet-inl.h \
    MurmurHash3.h \
    FileIo.h
