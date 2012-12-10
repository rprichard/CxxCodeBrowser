#
# Modify build settings to link against Clang -- update the include path,
# update the LIBS list, suppress diagnostics from the Clang headers, and
# configure C++ backend options.
#

include(./check-clang.pri)

INCLUDEPATH += $${CLANG_DIR}/include
unix: QMAKE_CXXFLAGS += -fPIC -pthread
QMAKE_CXXFLAGS += -fvisibility-inlines-hidden -fno-rtti -fno-exceptions
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter -Wno-strict-aliasing
win32: QMAKE_CXXFLAGS_WARN_ON += -Wno-enum-compare
DEFINES += _GNU_SOURCE __STDC_CONSTANT_MACROS __STDC_FORMAT_MACROS __STDC_LIMIT_MACROS

LIBS += -L$${CLANG_DIR}/lib
for(CLANG_LIB, CLANG_LIBS) {
    LIBS += -l$${CLANG_LIB}
}

linux-*: LIBS += -ldl
win32: LIBS += -lpsapi -limagehlp
