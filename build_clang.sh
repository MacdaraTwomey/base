#!/bin/bash

# Usage: build_clang.sh [os] [linux-output-path-type]

ARGC=$#

OS="win"
LINUX_OUTPUT_PATH_TYPE="linux-path"
if [[ $ARGC -gt 2 ]]
then
    echo "Error: Too many arguments passed to script"
    exit 1
elif [[ $ARGC -ge 1 ]]
then
    OS=$1

    if [[ $ARGC -eq 2 ]]
    then
        LINUX_OUTPUT_PATH_TYPE=$2
        if [[ $OS == "win" ]]
        then
            # When OS is windows, we always want windows filepaths
            echo "Error: Can't set output file path when building with windows"
            exit 1
        fi
    fi
fi



ROOT=$PWD
SOURCE="$ROOT/src/app.cpp"
FLAGS="-DBASE_DEBUG=1 -O2 -std=c++20"
INCLUDE="-I $ROOT/deps"
DISABLED_WARNINGS=" \
-Wno-unused-variable \
-Wno-gnu-anonymous-struct \
-Wno-nested-anon-types \
-Wno-gnu-zero-variadic-macro-arguments \
-Wno-missing-braces \
-fno-strict-aliasing \
-Wno-unused-function \
-Wno-language-extension-token \
-Wno-writable-strings"

if [[ $OS == "win" ]]
then
    CC="clang++.exe"

	LIBS="-lopengl32.lib -lkernel32.lib -l user32.lib -lgdi32.lib"

	# -m uses '/' instead of '\'
    SOURCE=`wslpath -m "$SOURCE"`
    INCLUDE=`wslpath -m "$INCLUDE"`
elif [[ $OS == "linux" ]]
then
    CC="clang++"
else
    echo "Error: Invalid OS '$OS'"
    exit 1
fi

pushd build > /dev/null

# -fdiagnostics-color forces colour even when piping to sed
CMD="$CC -o test.exe -g -Wall -pedantic-errors $DISABLED_WARNINGS $SOURCE $FLAGS $INCLUDE $LIBS"
echo $CMD

if [[ $LINUX_OUTPUT_PATH_TYPE == "win-path" ]]
then
	# Fixup error paths so we can jump to error on WSL
    eval "$CMD" 2>&1 | sed 's/\/mnt\/c/c:/g'
else
	eval "$CMD"
fi

popd > /dev/null
