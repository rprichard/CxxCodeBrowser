#
# Attempt to verify that the CLANG_DIR variable is set and points to a valid
# Clang installation.
#

REQUIRED_CLANG_VERSION = 3.1

equals(CLANG_DIR, "") {
    warning("The CLANG_DIR qmake variable is unset.")
    warning("Add CLANG_DIR=<path-to-clang-root> to the qmake command-line.")
    warning("The provided path should point to a" $$REQUIRED_CLANG_VERSION \
            "Clang installation.")
    warning("(In the QtCreator IDE, add the setting in the Projects mode.)")
    error("check-clang.pri: CLANG_DIR is unset.  Aborting.")
}

defineTest(checkClangRequire) {
    !exists($$1): error("check-clang.pri: Clang file $${1} does not exist.")
}

# Spot-check to make sure each kind of file exists -- binaries, headers, and
# libraries.
unix: checkClangRequire($${CLANG_DIR}/bin/clang)
unix: checkClangRequire($${CLANG_DIR}/bin/clang++)
win32: checkClangRequire($${CLANG_DIR}/bin/clang.exe)
win32: checkClangRequire($${CLANG_DIR}/bin/clang++.exe)
checkClangRequire($${CLANG_DIR}/include/clang/AST/ASTContext.h)

# Check that all expected libraries are present.
CLANG_LIBS = \
    clangFrontend clangSerialization clangDriver \
    clangTooling clangParse clangSema clangAnalysis \
    clangEdit clangAST clangLex clangBasic \
    LLVMMC LLVMObject LLVMSupport
for(CLANG_LIB, CLANG_LIBS) {
    checkClangRequire($${CLANG_DIR}/lib/lib$${CLANG_LIB}.a)
}
