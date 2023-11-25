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

SOURCE="$ROOT/code/win32.cpp"
INCLUDE="$ROOT/deps"
LIBS="-lopengl32.lib -lkernel32.lib -l user32.lib -lgdi32.lib"
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
    SOURCE=`wslpath -w "$SOURCE"`
    INCLUDE=`wslpath -w "$INCLUDE"`
elif [[ $OS == "linux" ]]
then
    CC="clang++"
else
    echo "Error: Invalid OS '$OS'"
    exit 1
fi

pushd build

set -x # echo commands

$CC -o base_test.exe -g -Wall -pedantic-errors -std=c++14 $DISABLED_WARNINGS $SOURCE -I $INCLUDE $LIBS

popd
