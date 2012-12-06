TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    third_party \
    libindexdb \
    clang-indexer \
    index-tool \
    navigator

linux-* {
    SUBDIRS += libbtrace
    btrace_script.path = /
    btrace_script.files += libbtrace/btrace.sh
    INSTALLS += btrace_script
    # HACK: Stop qmake from attempting to strip btrace.sh.
    QMAKE_STRIP = /bin/true
}

include(./check-clang.pri)
