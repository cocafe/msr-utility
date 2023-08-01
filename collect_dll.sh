#!/bin/bash

PATH=/${MSYS2_HOME/:/}/usr/bin/:$PATH

EXE=$1
shift
DLL_PATH=$@

if [ -z ${EXE} ]; then
	echo "program name is required"
	exit 1
fi

# resolve not found dll first
while :; do
	# if no unresolved dll
	ldd ${EXE} | grep -q "not found"
	if [ $? -ne 0 ]; then
		break;
	fi

	if [ -z ${DLL_PATH} ]; then
		echo "following dll is not found, and search path is empty:"
		ldd ${EXE} | grep "not found"
		exit 2
	fi

	# restart over if resolved one
	for i in `ldd ${EXE} | grep "not found" | awk '{print $1}'`; do
		find ${DLL_PATH} -name "$i" | grep -q "."
		if [ $? -ne 0 ]; then
			echo "$i is NOT found in dll path: ${DLL_PATH}"
			exit 2
		fi

		find ${DLL_PATH} -name "$i" -print0 | xargs -0 -I '{}' cp -v '{}' .
	done
done

for i in `ldd ${EXE} | grep "=> /mingw" | awk '{print $3}'`; do
	cp -v "$i" .
done
