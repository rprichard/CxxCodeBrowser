CONFIG += static

TARGET = indexdb
TEMPLATE = lib

SOURCES += \
    Buffer.cc \
    FileIo.cc \
    FileIo64BitSupport.cc \
    IndexArchiveBuilder.cc \
    IndexArchiveReader.cc \
    IndexDb.cc \
    MurmurHash3.cpp \
    StringTable.cc \
    sha2.c

HEADERS += \
    Buffer.h \
    Endian.h \
    FileIo.h \
    FileIo64BitSupport.h \
    IndexArchiveBuilder.h \
    IndexArchiveReader.h \
    IndexDb.h \
    MurmurHash3.h \
    StringTable.h \
    Util.h \
    sha2.h

INCLUDEPATH += ../third_party/libsnappy

QMAKE_CXXFLAGS += -std=c++0x
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
