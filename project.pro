TEMPLATE = subdirs

# Require that dependencies are built before building their users.
CONFIG += ordered

SUBDIRS += libindexdb \
           navigator \
           clang-test \
           clang-index-test \
           clang-indexer \
           libbtrace

btrace_script.path = /
btrace_script.files += libbtrace/btrace.sh
INSTALLS += btrace_script

# HACK: Stop qmake from attempting to strip btrace.sh.
QMAKE_STRIP = true
