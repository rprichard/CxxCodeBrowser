include(./config.pri)

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
    btrace_script.path = $$BINDIR
    btrace_script.files += \
        libbtrace/btrace.sh \
        libbtrace/processlog.py
    INSTALLS += btrace_script
    # HACK: Stop qmake from attempting to strip the scripts.
    QMAKE_STRIP = /bin/true
}

include(./check-clang.pri)
