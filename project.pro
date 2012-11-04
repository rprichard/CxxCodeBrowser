TEMPLATE = subdirs

# Require that sub-directories are built in the listed order, which will ensure
# that dependencies are built first.
CONFIG += ordered

SUBDIRS += \
    third_party \
    libbtrace \
    libindexdb \
    clang-indexer \
    index-tool \
    navigator

btrace_script.path = /
btrace_script.files += libbtrace/btrace.sh
INSTALLS += btrace_script

# HACK: Stop qmake from attempting to strip btrace.sh.
QMAKE_STRIP = /bin/true
