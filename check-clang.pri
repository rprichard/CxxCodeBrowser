#
# Attempt to verify that the LLVM_DIR variable is set and points to a valid
# Clang installation.
#

REQUIRED_LLVM_VERSION = 3.1

equals(LLVM_DIR, "") {
    warning("The LLVM_DIR qmake variable is unset.")
    warning("Add LLVM_DIR=<path-to-llvm-root> to the qmake command-line.")
    warning("The provided path should point to a" $$REQUIRED_LLVM_VERSION \
            "LLVM installation.")
    warning("It should not point to an LLVM build directory.")
    warning("It will contain bin, lib, and include subdirectories.")
    warning("(In the QtCreator IDE, add the setting in the Projects mode.)")
    error("LLVM_DIR is unset.  Aborting.")
}

defineTest(findClangRequire) {
    !exists($$1): error("LLVM+Clang file $${1} does not exist.")
}

# Spot-check to make sure each kind of file exists -- binaries, headers, and
# libraries.
unix: findClangRequire($${LLVM_DIR}/bin/clang)
unix: findClangRequire($${LLVM_DIR}/bin/clang++)
win32: findClangRequire($${LLVM_DIR}/bin/clang.exe)
win32: findClangRequire($${LLVM_DIR}/bin/clang++.exe)
findClangRequire($${LLVM_DIR}/include/clang/AST/ASTContext.h)

# Check that all expected libraries are present.
CLANG_LIBS = \
    clangFrontend clangSerialization clangDriver \
    clangTooling clangParse clangSema clangAnalysis \
    clangEdit clangAST clangLex clangBasic \
    LLVMMC LLVMObject LLVMSupport
for(CLANG_LIB, CLANG_LIBS) {
    findClangRequire($${LLVM_DIR}/lib/lib$${CLANG_LIB}.a)
}
