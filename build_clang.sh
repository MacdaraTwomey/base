#!/bin/bash

ARGC=$#
if [[ $ARGC -eq 0 ]]
then
    # Default to windows
    OS="win"
elif [[ $ARGC -eq 1 ]]
then
    OS=$1
else
    echo "Error: Too many arguments passed to script"
    exit 1
fi


ROOT=$PWD

INCLUDE="$ROOT/deps"
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
	# -m uses '/' instead of '\'
    SOURCE=`wslpath -m "$ROOT/code/win32.cpp"`
    INCLUDE=`wslpath -m "$INCLUDE"`
	LIBS="-lopengl32.lib -lkernel32.lib -l user32.lib -lgdi32.lib"
elif [[ $OS == "linux" ]]
then
    CC="clang++"
    SOURCE="$ROOT/code/linux.cpp"
else
    echo "Error: Invalid OS '$OS'"
    exit 1
fi

pushd build

CMD="$CC -o base_test.exe -g -Wall -pedantic-errors -std=c++14 $DISABLED_WARNINGS $SOURCE -I $INCLUDE $LIBS"

if [[ $OS == "linux" ]]
then
	# Fixup error paths so we can jump to error on WSL
    eval "$CMD" 2>&1 | sed 's/\/mnt\/c/c:/g'
else
	eval "$CMD"
fi

popd
