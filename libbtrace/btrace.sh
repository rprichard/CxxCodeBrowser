#!/bin/bash
TOP=$(cd $(dirname $0) && pwd)
export LD_PRELOAD=$TOP/libbtrace.so
export BTRACE_LOG=$PWD/btrace.log
"$@"
