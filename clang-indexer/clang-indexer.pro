include(../config.pri)

QT += core
QT -= gui

TARGET = sw-clang-indexer
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle

SOURCES += \
    ASTIndexer.cc \
    DaemonPool.cc \
    IndexBuilder.cc \
    IndexerContext.cc \
    IndexerPPCallbacks.cc \
    Mutex.cc \
    NameGenerator.cc \
    Process.cc \
    TUIndexer.cc \
    Util.cc \
    main.cc

HEADERS += \
    ASTIndexer.h \
    DaemonPool.h \
    IndexBuilder.h \
    IndexerContext.h \
    IndexerPPCallbacks.h \
    Location.h \
    Mutex.h \
    NameGenerator.h \
    Process.h \
    Switcher.h \
    TUIndexer.h \
    Util.h

OTHER_FILES += \
    dependencies.cfg

DEFINES += JSON_IS_AMALGAMATION

ROOT_DIR = ..
include(../add_dependencies.pri)

linux-* {
    target.path = $$LIBEXEC_DIR
} else {
    target.path = $$BIN_DIR
}
INSTALLS += target

include(../enable-cxx11.pri)

# On Win32, we must use the gnu++0x dialect so that isascii is defined in
# cctype.  Clang's Expr.h uses isascii.
win32 {
    QMAKE_CXXFLAGS -= -std=c++0x
    QMAKE_CXXFLAGS += -std=gnu++0x
    QMAKE_LFLAGS -= -std=c++0x
    QMAKE_LFLAGS += -std=gnu++0x
}
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter -Wno-uninitialized
QMAKE_CXXFLAGS += -Wno-reserved-user-defined-literal

include(../link-clang.pri)

DEFINES += INDEXER_CLANG_DIR=$${CLANG_DIR}
