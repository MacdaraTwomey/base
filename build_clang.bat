@echo off


set SourceFiles=..\code\win32.cpp

set IncludeDirs=

set LinkedLibraries=-lopengl32.lib -lgdi32.lib -luser32.lib

pushd build

:: -pedantic-errors
clang++  c:\dev\base\code\win32.cpp -Wall -Wno-unused-variable -fno-strict-aliasing -g -o base_test.exe %IncludeDirs% %LinkedLibraries%


popd
