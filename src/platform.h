#pragma once

#include "base.h"

///////////////////////////////////////////////////////////////////////
// File Handling
//

// TODO: File handle API, with error conditions available via a call to PlatformGetFileError or something.
// TODO: Handle deleteing directories

bool                   PlatformFileExists(string FilePath);
u64                    PlatformGetFileSize(string FilePath);
string                 PlatformGetExecutablePath(arena *Arena);

string                 PlatformReadEntireFile(arena *Arena, string FilePath);
bool                   PlatformWriteEntireFile(u64 Size, u8 *Contents, string FilePath);

bool                   PlatformDeleteFile(string FilePath);

///////////////////////////////////////////////////////////////////////
// Memory
//

void *  PlatformMemoryReserve(u64 Size);
bool    PlatformMemoryCommit(void *Address, u64 size);
void    PlatformMemoryDecommit(void *Address, u64 Size);

// Size must match the original reserved Size. 
// On Windows Size is not used, the entire allocation is freed. 
// On Linux Size is used to determine how many pages to free.
void    PlatformMemoryFree(void *Address, u64 Size);
void    PlatformMemoryGuard(void *Address, u64 Size);
void    PlatformMemoryRemoveGuard(void *Address, u64 Size);