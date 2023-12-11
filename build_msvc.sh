
ROOT=`wslpath -w $PWD`

SOURCE="$ROOT\src\win32.cpp"
INCLUDE="-I$ROOT\deps"
LIBS="opengl32.lib kernel32.lib user32.lib gdi32.lib"

pushd build

cl.exe $SOURCE $INCLUDE -DBASE_DEBUG=1 -W3 -std:c++17 -wd4201 -wd4505 -INCREMENTAL:NO -FC -EHs- -nologo -Zi -link $LIBS -out:test.exe 

popd
