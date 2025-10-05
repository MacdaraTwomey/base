#pragma once

#include "base.h"

///////////////////////////////////////////////////////////////////////
// File Handling
//

// TODO: File handle API, with error conditions available via a call to OS_GetFileError or something.
// TODO: Handle deleteing directories

typedef s32 os_file_handle;

enum os_file_info {
    OS_FileInfo_Error,
    OS_FileInfo_File,
    OS_FileInfo_Directory,
};

struct os_directory_iterator {
    void *Handle;
};

struct os_filesystem_entry {
    string Name;
    os_file_info Info;
};

bool                    OS_FileExists(string FilePath);
os_file_info            OS_GetFileInfo(string Path);
u64                     OS_GetFileSize(string FilePath);
string                  OS_GetExecutablePath(arena *Arena);
string                  OS_CanonicalAbsolutePath(arena *Arena, string Path);

os_directory_iterator   OS_DirectoryIterator(string DirectoryPath);
void                    OS_DirectoryIteratorClose(os_directory_iterator *Iter);
bool                    OS_DirectoryIteratorIsOk(os_directory_iterator Iter);
os_filesystem_entry     OS_DirectoryIteratorNext(arena *Arena, os_directory_iterator *Iter);

string                  OS_ReadEntireFile(arena *Arena, string FilePath);
bool                    OS_WriteEntireFile(u64 Size, u8 *Contents, string FilePath);

bool                    OS_DeleteFile(string FilePath);
void                    OS_CloseFile(os_file_handle Handle);


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