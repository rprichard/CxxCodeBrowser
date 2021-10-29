#!/bin/sh
#
# This script will create a bigint-2010.04.30 subdirectory in the current
# directory.  CxxCodeBrowser's bin directory must be in the PATH.
#

set -e
script_dir=$(cd "$(dirname "$0")" && pwd)

checkTool() {
    if ! which "$1" >/dev/null 2>/dev/null; then
        echo "$0: error: The $1 program must be in the PATH."
        exit 1
    fi
}

checkTool ccb-btrace
checkTool ccb-btrace-to-compiledb
checkTool ccb-clang-indexer
checkTool CxxCodeBrowser

set -x
rm -fr bigint-2010.04.30
tar -xf "$script_dir/bigint-2010.04.30.tar.bz2"
cd bigint-2010.04.30
ccb-btrace make -j4 all testsuite
ccb-btrace-to-compiledb
ccb-clang-indexer --index-project
CxxCodeBrowser index
