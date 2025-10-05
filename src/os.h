#pragma once

#include "base.h"

///////////////////////////////////////////////////////////////////////
// File Handling
//

// TODO: File handle API, with error conditions available via a call to OS_GetFileError or something.
// TODO: Handle deleteing directories

typedef s32 os_file_handle;

void OS_CloseFile(os_file_handle Handle);

bool                   OS_FileExists(string FilePath);
u64                    OS_GetFileSize(string FilePath);
string                 OS_GetExecutablePath(arena *Arena);

string                 OS_ReadEntireFile(arena *Arena, string FilePath);
bool                   OS_WriteEntireFile(u64 Size, u8 *Contents, string FilePath);

bool                   OS_DeleteFile(string FilePath);

///////////////////////////////////////////////////////////////////////
// Memory
//

void *  OS_MemoryReserve(u64 Size);
bool    OS_MemoryCommit(void *Address, u64 size);
void    OS_MemoryDecommit(void *Address, u64 Size);

// Size must match the original reserved Size. 
// On Windows Size is not used, the entire allocation is freed. 
// On Linux Size is used to determine how many pages to free.
void    OS_MemoryFree(void *Address, u64 Size);
void    OS_MemoryGuard(void *Address, u64 Size);
void    OS_MemoryRemoveGuard(void *Address, u64 Size);


// Time

u64 OS_GetWallClockMicroseconds();