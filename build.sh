#!/bin/bash
set -e
cd $(dirname $0)
mkdir -p build
cd build
~/QtSDK/Desktop/Qt/4.8.1/gcc/bin/qmake ..
export INSTALL_ROOT=$PWD/../install
make
make install
