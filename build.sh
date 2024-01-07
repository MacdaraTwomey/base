#!/bin/bash

# Usage: 
# build.sh win      
# build.sh linux [win-path]
# build.sh msvc

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
        if [[ $OS == "win" || $OS == "msvc" ]]
        then
            # When OS is windows, we always want windows filepaths
            echo "Error: Can't set output file path when building with windows"
            exit 1
        fi
    fi
fi

function wsl_to_win() {
	echo "$1" | sed 's/\/mnt\/c/c:/g'
}

ROOT=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

SRC="$ROOT/src/app.cpp"
INCLUDE="-I $ROOT/deps"
TARGET="-o test.exe"

FLAGS="-g -DBASE_DEBUG=1 -std=c++20"
#FLAGS+=" -fsanitize=address"
FLAGS+=" -Wall -pedantic-errors"
FLAGS+=" -Wno-unused-variable"
FLAGS+=" -Wno-gnu-anonymous-struct"
FLAGS+=" -Wno-nested-anon-types"
FLAGS+=" -Wno-gnu-zero-variadic-macro-arguments"
FLAGS+=" -Wno-missing-braces"
FLAGS+=" -fno-strict-aliasing"
FLAGS+=" -Wno-unused-function"
FLAGS+=" -Wno-language-extension-token"
FLAGS+=" -Wno-writable-strings"
#FLAGS+=" -fdiagnostics-color"    # forces colour even when piping to sed

if [[ $OS == "win" ]]
then
    CC="clang++.exe"
	LIBS="-l opengl32.lib -l kernel32.lib -l user32.lib -l gdi32.lib"
	SRC=`wsl_to_win "$SRC"`
	INCLUDE=`wsl_to_win "$INCLUDE"`
elif [[ $OS == "msvc" ]]
then
    CC="cl.exe"
	LIBS="-link opengl32.lib kernel32.lib user32.lib gdi32.lib"
	SRC=`wsl_to_win "$SRC"`
	INCLUDE=`wsl_to_win "$INCLUDE"`

	# wd5105 because the new MSVC preprocessor warns on microsoft header
	FLAGS="-DBASE_DEBUG=1 -W3 -std:c++latest /Zc:preprocessor -Zc:strictStrings- -wd5105 -wd4201 -wd4505 -INCREMENTAL:NO -FC -EHs- -nologo -Zi"
	# This needs to be after LIBS (which has the -link flag) for msvc to work
	TARGET="-out:test.exe"
elif [[ $OS == "linux" ]]
then
    CC="clang++"
else
    echo "Error: Invalid OS '$OS'"
    exit 1
fi

pushd build > /dev/null

CMD="$CC $SRC $INCLUDE $FLAGS $LIBS $TARGET"
echo "$CMD"

if [[ $LINUX_OUTPUT_PATH_TYPE == "win-path" ]]
then
	# Fixup error paths so we can jump to error on WSL
	OUTPUT=`eval "$CMD" 2>&1`
	wsl_to_win "$OUTPUT"
else
	eval "$CMD"
fi

popd > /dev/null
