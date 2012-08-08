TEMPLATE = subdirs

SUBDIRS += navigator \
           clang-test \
           clang-index-test \
           libbtrace

btrace_script.path = /
btrace_script.files += libbtrace/btrace.sh
INSTALLS += btrace_script

# HACK: Stop qmake from attempting to strip btrace.sh.
QMAKE_STRIP = true
