#
# Build against a Clang 3.1 install directory.
#

LLVM_DIR=/home/rprichard/llvm-3.1-configure-release-install

INCLUDEPATH += $${LLVM_DIR}/include
unix: QMAKE_CXXFLAGS += -fPIC -pthread
QMAKE_CXXFLAGS += -fvisibility-inlines-hidden -fno-rtti -fno-exceptions
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter -Wno-strict-aliasing
win32: QMAKE_CXXFLAGS_WARN_ON += -Wno-enum-compare
DEFINES += _GNU_SOURCE __STDC_CONSTANT_MACROS __STDC_FORMAT_MACROS __STDC_LIMIT_MACROS

LIBS += -L$${LLVM_DIR}/lib

LIBS += -lclangFrontend -lclangSerialization -lclangDriver \
           -lclangTooling -lclangParse -lclangSema -lclangAnalysis \
           -lclangEdit -lclangAST -lclangLex -lclangBasic

LIBS += -lLLVMMC -lLLVMObject -lLLVMSupport

unix: LIBS += -ldl
win32: LIBS += -lpsapi -limagehlp
