#
# Build against a Clang 3.1 install directory.
#

LLVM_DIR=/home/rprichard/llvm-3.1-configure-release-install

INCLUDEPATH += $${LLVM_DIR}/include
QMAKE_CXXFLAGS += -fPIC -fvisibility-inlines-hidden -fno-rtti -fno-exceptions -pthread -Wno-unused-parameter -Wno-strict-aliasing
DEFINES += _GNU_SOURCE __STDC_CONSTANT_MACROS __STDC_FORMAT_MACROS __STDC_LIMIT_MACROS

LIBS += \
    -ldl \
    -L$${LLVM_DIR}/lib

LIBS += -lclangFrontend -lclangSerialization -lclangDriver \
           -lclangTooling -lclangParse -lclangSema -lclangAnalysis \
           -lclangEdit -lclangAST -lclangLex -lclangBasic

LIBS += -lLLVMMC -lLLVMObject -lLLVMSupport
