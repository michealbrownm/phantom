#!/bin/bash

SYSTEM=`uname -s`
OS_INFO=linux-gcc
if [ "$SYSTEM"x == "Darwin"x ]  ; then
echo $SYSTEM ...
OS_INFO=mac-clang
fi

basepath=$(cd `dirname $0`; pwd)
GCC_VERSION=$(gcc -dumpversion)
export MYSCONS=${basepath}"/scons-2.1.0/"
echo $MYSCONS
export SCONS_LIB_DIR=$MYSCONS/engine
python $MYSCONS/script/scons platform=$OS_INFO $*
#mv "libs/$OS_INFO-${GCC_VERSION}/libjson_$OS_INFO-${GCC_VERSION}_libmt.so" libs/libjson.so
mv "libs/$OS_INFO-${GCC_VERSION}/libjson_$OS_INFO-${GCC_VERSION}_libmt.a" libs/libjson.a

