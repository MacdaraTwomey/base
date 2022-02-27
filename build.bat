@echo off


set SourceFiles=..\code\win32.cpp

set IncludeDirs=

set LinkedLibraries=opengl32.lib gdi32.lib user32.lib

pushd build

::c++ standard flag

cl %SourceFiles% %IncludeDirs% -DBASE_DEBUG=1 -W4 -INCREMENTAL:NO -FC -EHs- -nologo -Zi -link %LinkedLibraries% -out:base_test.exe 


popd
