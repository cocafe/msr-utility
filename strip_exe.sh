#!/bin/bash

PATH=/${MSYS2_HOME/:/}/usr/bin/:$PATH

EXE=$1
STRIPPED_EXE=$(basename --suffix .exe ${EXE})_stripped.exe

cp -v ${EXE} ${STRIPPED_EXE}
strip --strip-all ${STRIPPED_EXE}
exit $?