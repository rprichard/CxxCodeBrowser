include(../config.pri)

QT -= core gui
CONFIG += dll

TARGET = sw-btrace
TEMPLATE = lib

SOURCES += btrace.c
linux-*: SOURCES += btrace_linux.c
freebsd-*: SOURCES += btrace_freebsd.c
darwin-*|macx-*: SOURCES += btrace_darwin.c

OTHER_FILES += \
    version_script

# It is never reasonable for the btrace preload library to have an RPATH,
# but it is reasonable for other programs in this project.  QtSDK's default
# configuration sets the RPATH to the SDK's lib directory.  Override the
# default by clearing the RPATH setting.
QMAKE_RPATHDIR =

linux-*|freebsd-* {
    QMAKE_LFLAGS += -Wl,--version-script,$$_PRO_FILE_PWD_/version_script
}

freebsd-* {
    QMAKE_LIBS += -lkvm
}

#QMAKE_CFLAGS_WARN_ON += -Wno-unused-result
include(../enable-c99.pri)

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
target.path = $$LIBEXEC_DIR
INSTALLS += target
