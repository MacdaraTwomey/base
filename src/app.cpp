
void AppMain(int ArgCount, char *Args[]);

#include "test.cpp"

#if BASE_OS_WINDOWS
#  include "win32.cpp"
#elif BASE_OS_LINUX
#  include "linux.cpp"
#endif

void AppMain(int ArgCount, char *Args[])
{
    RunTests();
}
