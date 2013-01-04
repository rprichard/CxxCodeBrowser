#!/bin/sh
#
# A wrapper script for a program.  Add paths to LD_LIBRARY_PATH if the wrapped
# program needs a newer libstdc++ than the one that would otherwise be found.
# The script should be duplicated for each program it wraps.  Assuming it is
# installed at <installdir>/<someprog>, the script will adjust LD_LIBRARY_PATH,
# then run <installdir>/../libexec/<someprog>.

self=$0
if test -L "$self"; then
    self=$(readlink -f "$self")
fi
bin_dir=$(cd "$(dirname "$self")" && pwd)
libexec_dir=$bin_dir/../libexec
program=$libexec_dir/$(basename "$self")
lib_cfg_file=$libexec_dir/sw-libstdcxx-path

run() {
    if ! "$@"; then
        echo "$0: warning: command failed: $@" >&2
        echo "$0: warning: Leaving LD_LIBRARY_PATH untouched" >&2
        return 1
    fi
    return 0
}

# SourceWeb may have been linked with a libstdc++ newer than the one on the
# host system (e.g. from clang-redist-linux's gcc-libs package).  In this case,
# one or more library paths may have been written to the sw-libstdcxx-path
# file.  We want to add these paths to LD_LIBRARY_PATH, but only if the host
# libstdc++ is too old.  Examine the version symbols in the program-to-run and
# in the default libstdc++.so.6 library and see if any version identifiers are
# missing.
if test -f "$lib_cfg_file"; then
    error=0
    if test $error -eq 0; then
        verdef=$(run "$libexec_dir/sw-linux-elfvinfo" --dlopen --verdef libstdc++.so.6)
        error=$?
    fi
    if test $error -eq 0; then
        verneed=$(run "$libexec_dir/sw-linux-elfvinfo" --verneed "$program")
        error=$?
    fi
    if test $error -eq 0; then
        verneed=$(echo "$verneed" | awk '/^libstdc\+\+\.so\.6\t/' | cut -f2)
        found=1
        for v1 in $verneed; do
            found=0
            for v2 in $verdef; do
                if test "$v1" = "$v2"; then
                    found=1
                    break
                fi
            done
            if test $found -eq 0; then break; fi
        done
        if test $found -eq 0; then
            LD_LIBRARY_PATH=$(cat "$lib_cfg_file")${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
            export LD_LIBRARY_PATH
        fi
    fi
fi

exec "$program" "$@"
