#!/bin/bash

ROOT=`wslpath -w $PWD`

SOURCE="$ROOT\code\win32.cpp"
INCLUDE="-I $ROOT\deps"
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

pushd build

#clang-cl.exe c:/dev/projects/base/code/win32.cpp -DBASE_DEBUG=1 -INCREMENTAL:NO -FC -EHs- -nologo -Zi -I../deps -link opengl32.lib kernel32.lib user32.lib gdi32.lib -out:reflect.exe

set -x # echo commands

clang++.exe -o base_test.exe -g -Wall -pedantic-errors -std=c++14 $DISABLED_WARNINGS $SOURCE $INCLUDE $LIBS

popd
