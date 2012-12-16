include(../config.pri)

QT -= core gui
CONFIG += dll

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

TARGET = btrace
TEMPLATE = lib

SOURCES += \
    libbtrace.c

OTHER_FILES += \
    version_script

DEFINES += _GNU_SOURCE

LIBS += -ldl

QMAKE_CFLAGS += -std=c99
QMAKE_LFLAGS += -nostartfiles
QMAKE_LFLAGS += -Wl,--version-script,$$_PRO_FILE_PWD_/version_script

# XXX: Perhaps libbtrace should be installed into a library directory, but then
# the btrace.sh has to locate the library somehow.  Perhaps it could look in
# ../lib.  Perhaps we could write a path into btrace.sh somehow at
# install-time?  Assuming there is a configurable --libdir/LIBDIR setting, the
# library will be placed into $(INSTALL_ROOT)/$$LIBDIR, but INSTALL_ROOT isn't
# know at qmake-time.  Perhaps we could forbid the use of INSTALL_ROOT.  I
# think the point of having separately-configurable BINDIR and LIBDIR options
# is to deal with multiarch systems.
#
# On a related subject, we need to consider how this LD_PRELOAD technique works
# on a 64-bit system where 32-bit processes run during the build.  I think the
# dynamic linker prints an error message when it sees the incompatible
# LD_PRELOAD library, but then continues running.  Apparently some versions of
# the dynamic linker recognize a magic '$LIB' string (like '$ORIGIN') and turn
# it into lib or lib64, but again, what about Debian multiarch systems?
# Another idea is to alter LD_PRELOAD inside libbtrace's exec wrappers
# depending upon the bit-ness of the program being exec'ed.  (It's possible to
# exec non-ELF images, though, like shebang, Win32, or Mono executables.)
target.path = $$BINDIR
INSTALLS += target

QMAKE_CFLAGS_WARN_ON += -Wno-unused-result
