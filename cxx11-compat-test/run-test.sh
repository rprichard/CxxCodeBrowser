#!/bin/sh

usage()
{
cat <<EOF
Usage: $0 qmake-command [optional-qmake-arguments]

Tests that a qmake configuration is working and has sufficient C++11 support.
This script will create a cxx11-compat-test-build directory in PWD.
EOF
}

if [ $# -eq 0 ]; then
    usage
    exit 0
fi

SRCDIR=$(cd "$(dirname "$0")" && pwd)
rm -fr cxx11-compat-test-build

set -e -x
mkdir cxx11-compat-test-build
cd cxx11-compat-test-build
"$@" "$SRCDIR/cxx11-compat-test.pro"
make
set +x

echo "$0: SUCCESS"
