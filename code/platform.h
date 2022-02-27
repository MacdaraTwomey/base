#pragma once

#include "base.h"

struct platform_file_contents
{
    u8 *Contents;
    size_t Size;
};

string PlatformGetExecutablePath(arena *Arena);
bool PlatformFileExists(string Filename);
u64 PlatformGetFileSize(string FileName);
platform_file_contents PlatformReadEntireFile(arena *Arena, string FileName);

void *PlatformMemoryReserve(u64 Size);
bool PlatformMemoryCommit(void *Address, u64 size);
void PlatformMemoryDecommit(void *Address, u64 Size);
void PlatformMemoryFree(void *Address);
