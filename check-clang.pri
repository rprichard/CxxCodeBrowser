#
# Attempt to verify that the CLANG_DIR variable is set and points to a valid
# Clang installation.
#

REQUIRED_CLANG_VERSION = 3.6

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

# Spot-check to make sure we can find Clang headers and libraries.  We do not
# need the clang/clang++ executables, and sometimes they do not exist.  (e.g.
# Ubuntu has multiple libllvm-3.X-dev packages, but only a single clang binary
# can exist at once.)

checkClangRequire($${CLANG_DIR}/include/clang/AST/ASTContext.h)

# Check that all expected libraries are present.
CLANG_LIBS = \
    clangFrontend clangSerialization clangDriver \
    clangTooling clangParse clangSema clangAnalysis \
    clangEdit clangAST clangLex clangBasic \
    LLVMMC LLVMMCParser LLVMObject LLVMAsmParser LLVMCore LLVMSupport \
    LLVMOption LLVMBitWriter LLVMBitReader
for(CLANG_LIB, CLANG_LIBS) {
    checkClangRequire($${CLANG_DIR}/lib/lib$${CLANG_LIB}.a)
}
