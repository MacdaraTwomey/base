#pragma once

#include "base.h"

///////////////////////////////////////////////////////////////////////
// File Handling
//

// TODO: File handle API, with error conditions available via a call to PlatformGetFileError or something.

struct platform_file_contents
{
    u8 *Contents;
    u64 Size;
};

bool                   PlatformFileExists(string FilePath);
u64                    PlatformGetFileSize(string FilePath);
string                 PlatformGetExecutablePath(arena *Arena);

platform_file_contents PlatformReadEntireFile(arena *Arena, string FilePath);
bool                   PlatformWriteEntireFile(u64 Size, u8 *Contents, string FilePath);

// TODO: Handle deleteing directories
bool                   PlatformDeleteFile(string FilePath);

///////////////////////////////////////////////////////////////////////
// Memory
//

void *  PlatformMemoryReserve(u64 Size);
bool    PlatformMemoryCommit(void *Address, u64 size);
void    PlatformMemoryDecommit(void *Address, u64 Size);
void    PlatformMemoryFree(void *Address);
void    PlatformMemoryGuard(void *Address, u64 Size);
void    PlatformMemoryRemoveGuard(void *Address, u64 Size);