@echo off


set SourceFiles=..\code\win32.cpp

set IncludeDirs=

set LinkedLibraries=-lopengl32.lib -lgdi32.lib -luser32.lib

pushd build

:: -pedantic-errors

:: GNU extensions used
:: -Wno-missing-braces   because clang suggests double braces around v2 initialiser v2{{1, 0}}
:: -Wno-gnu-anonymous-struct -Wno-nested-anon-types  for allowing anonymous structs for  v2, v3, v4
:: -Wno-gnu-zero-variadic-macro-arguments  to allow ## before __VA_ARGS__ to eat comma 

clang++  c:\dev\projects\base\code\win32.cpp -Wall  -pedantic-errors -std=c++14 -Wno-unused-variable -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-gnu-zero-variadic-macro-arguments -Wno-missing-braces -fno-strict-aliasing -g -o base_test.exe %IncludeDirs% %LinkedLibraries%


popd
