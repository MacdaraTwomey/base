
#include "test.cpp"

#if BASE_OS_WINDOWS
#  include "win32.cpp"
#elif BASE_OS_LINUX
#  include "linux.cpp"
#endif

int main(int ArgCount, char *Args[]) 
{
    RunTests();
}
