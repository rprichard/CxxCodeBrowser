CONFIG += static

TARGET = indexdb
TEMPLATE = lib

SOURCES += \
    Buffer.cc \
    FileIo.cc \
    IndexDb.cc \
    StringTable.cc \
    MurmurHash3.cpp

HEADERS += \
    Buffer.h \
    FileIo.h \
    IndexDb.h \
    StringTable.h \
    MurmurHash3.h

QMAKE_CXXFLAGS += -std=c++0x
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
