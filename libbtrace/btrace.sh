#!/bin/sh

self=$0
if test -L "$self"; then
    self=$(readlink -f "$self")
fi
bin_dir=$(cd "$(dirname "$self")" && pwd)
libexec_dir=$bin_dir/../libexec

# This code copies fakeroot's behavior by adding the library to the front of the
# space-delimited LD_PRELOAD variable.
#
# TODO: It should follow fakeroot more closely by setting LD_LIBRARY_PATH to
# include both a 32-bit and (on appropriate hosts) a 64-bit library directory.
# It could then provide just the library basename to LD_PRELOAD, and the
# dynamic linker would pick an architecture-appropriate library, instead of
# warning about an incompatible library, as it currently does.

preload_lib=$libexec_dir/libbtrace.so
LD_PRELOAD=$preload_lib${LD_PRELOAD:+:$LD_PRELOAD}
BTRACE_LOG=$PWD/btrace.log
export LD_PRELOAD
export BTRACE_LOG

"$@"
