# HACK: Make qmake use the C driver rather than the C++ driver.  This line does
# not use QMAKE_LINK_C because some mkspec files don't define it (e.g. Clang).
# Note that even with this line, and when using Clang, the linker command-line
# begins with:
#    clang -ccc-gcc-name g++ ...
# The g++ is worrisome, but the setting still successfully prevents the shared
# object from linking against libstdc++.
QMAKE_LINK = $$QMAKE_CC

# HACK: qmake passes -ldl -lpthread to the linker by default.  Suppress
# -lpthread by clearing QMAKE_LIBS_THREAD.  From examination of the mkspec
# files, I would expect that clearing QMAKE_LIBS_DYNLOAD would suppress -ldl,
# but it doesn't work.
QMAKE_LIBS_THREAD =

QMAKE_CFLAGS += -std=gnu99
