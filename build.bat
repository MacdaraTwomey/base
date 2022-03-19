@echo off


set SourceFiles=..\code\win32.cpp

set IncludeDirs=-I..\deps

set LinkedLibraries=opengl32.lib gdi32.lib user32.lib 

pushd build

:: -wd4201 disable  nonstandard extension used: nameless struct/union
:: -wd4505 disable  unreferenced local function has been removed
::c++ standard flag

cl %SourceFiles% %IncludeDirs% -DBASE_DEBUG=1 -W4 -std:c++14 -wd4201 -wd4505 -INCREMENTAL:NO -FC -EHs- -nologo -Zi -link %LinkedLibraries% -out:base_test.exe 


popd
