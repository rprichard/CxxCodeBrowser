CONFIG += static

TARGET = indexdb
TEMPLATE = lib

SOURCES += \
    Buffer.cc \
    FileIo.cc \
    IndexDb.cc \
    StringTable.cc \
    MurmurHash3.cpp \
    IndexArchiveReader.cc \
    IndexArchiveBuilder.cc \
    FileIo64BitSupport.cc \
    sha2.c

HEADERS += \
    Buffer.h \
    FileIo.h \
    IndexDb.h \
    StringTable.h \
    MurmurHash3.h \
    Util.h \
    Endian.h \
    IndexArchiveReader.h \
    IndexArchiveBuilder.h \
    FileIo64BitSupport.h \
    sha2.h

QMAKE_CXXFLAGS += -std=c++0x
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
