#!/bin/sh
#
# This script will create a bigint-2010.04.30 subdirectory in the current
# directory.  SourceWeb's bin directory must be in the PATH.
#

set -e
script_dir=$(cd "$(dirname "$0")" && pwd)

checkTool() {
    if ! which "$1" >/dev/null 2>/dev/null; then
        echo "$0: error: The $1 program must be in the PATH."
        exit 1
    fi
}

checkTool sw-btrace
checkTool sw-btrace-to-compiledb
checkTool sw-clang-indexer
checkTool sourceweb

set -x
rm -fr bigint-2010.04.30
tar -xf "$script_dir/bigint-2010.04.30.tar.bz2"
cd bigint-2010.04.30
sw-btrace make -j4 all testsuite
sw-btrace-to-compiledb
sw-clang-indexer --index-project
sourceweb
