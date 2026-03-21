
#include "base.cpp"

#if BASE_OS_WINDOWS
#  define WIN32_GUI_APP 0
#  include "win32.cpp"
   // I include this after the os functions as these define and load a bunch of opengl function pointers
#  include "opengl.cpp"
#elif BASE_OS_LINUX
#  include "linux.cpp"
#endif

#include "test/test.cpp"

int AppMain(int ArgCount, char *Args[])
{
    RunTests();
    return 0;
}
