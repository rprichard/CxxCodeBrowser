#!/bin/bash
TOP=$(cd $(dirname $0) && pwd)

# This code copies fakeroot's behavior by adding the library to the front of the
# space-delimited LD_PRELOAD variable.
#
# TODO: It should follow fakeroot more closely by setting LD_LIBRARY_PATH to
# include both a 32-bit and (on appropriate hosts) a 64-bit library directory.
# It could then provide just the library basename to LD_PRELOAD, and the
# dynamic linker would pick an architecture-appropriate library, instead of
# warning about an incompatible library, as it currently does.

LIB=$TOP/libbtrace.so
SEP=""
if [ -n "$LD_PRELOAD" ]; then
    SEP=" "
fi
export LD_PRELOAD="$LIB$SEP$LD_PRELOAD"

export BTRACE_LOG=$PWD/btrace.log

"$@"
