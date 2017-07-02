include(./config.pri)

TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    third_party \
    libindexdb \
    clang-indexer \
    index-tool \
    navigator

# HACK: Stop qmake from attempting to strip the scripts.
QMAKE_STRIP = /bin/echo

# Unix btrace.
linux-*|freebsd-*|darwin-*|macx-* {
    SUBDIRS += btrace
    btrace_script.path = $$BIN_DIR
    btrace_script.files += \
        btrace/sw-btrace \
        btrace/sw-btrace-to-compiledb
    INSTALLS += btrace_script
}

include(./check-clang.pri)

