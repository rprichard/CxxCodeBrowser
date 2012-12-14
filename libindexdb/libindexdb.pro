QT -= core gui
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
    StringTable.cc

HEADERS += \
    Buffer.h \
    Endian.h \
    FileIo.h \
    FileIo64BitSupport.h \
    IndexArchiveBuilder.h \
    IndexArchiveReader.h \
    IndexDb.h \
    StringTable.h \
    Util.h \
    WriterSha256Context.h

OTHER_FILES += \
    dependencies.cfg

ROOT_DIR = ..
include(../add_dependencies.pri)

include(../enable-cxx11.pri)
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
