
#include "os.h"
#include "base.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <dirent.h> // closedir, opendir

#include <time.h> // clock_gettime, CLOCK_MONOTONIC

// TODO: Maybe allow direcotries to return true
// If we do this, then we can use access().
bool OS_FileExists(string FilePath)
{
    temp_arena Scratch = GetScratch();
    u8 *PathZ  = PushCString(Scratch.Arena, FilePath);
    
    bool Exists = false;
    
    struct stat64 Info = {};
    if (stat64((char *)PathZ, &Info) == 0)
    {
        Exists = !S_ISDIR(Info.st_mode);
    }
    
    ReleaseScratch(Scratch);
    
    return Exists;
}

os_file_info OS_GetFileInfo(string Path) {
    temp_arena Scratch = GetScratch();

    u8 *NullTerminatedPath = PushCString(Scratch.Arena, Path);

    os_file_info Info = OS_FileInfo_Error;
    struct stat Stat = {};
    int Result = stat((char *)NullTerminatedPath, &Stat);
    if (Result == 0) {
        // Everythings thats not a dir is a file
        // If its a symlink stat should return info about the thing that is linked
        Info = S_ISDIR(Stat.st_mode) ? OS_FileInfo_Directory : OS_FileInfo_File;
    }

    ReleaseScratch(Scratch);

    return Info;
}

u64 OS_GetFileSize(string FilePath)
{
    temp_arena Scratch = GetScratch();
    u8 *PathZ  = PushCString(Scratch.Arena, FilePath);
    
    u64 Size = 0;
    
    // stat() will follow links so the stat()'ed file should never be a symbolic link.
    // It could be a directory, pipe, socket, character device, fifo though.
    struct stat64 Info = {};
    if (stat64((char *)PathZ, &Info) == 0)
    {
        Assert(S_ISREG(Info.st_mode));
        Size = Info.st_size;
    }
    
    ReleaseScratch(Scratch);
    
    return Size;
}

string OS_GetExecutablePath(arena *Arena)
{
    string Path = {};
    
    u64 BufferSize = KB(8);
    u8 *Buffer = PushArrayNoZero(Arena, BufferSize, u8);
    ssize_t PathLength = readlink("/proc/self/exe", (char *)Buffer, BufferSize);
    if (PathLength != -1)
    {
        u64 Excess = BufferSize - PathLength;
        PopSize(Arena, Excess);
        Path = CreateString(Buffer, PathLength);
    }
    
    return Path;
}

string OS_CanonicalAbsolutePath(arena *Arena, string Path) {
    string Result = {};

    temp_arena Scratch = GetScratch(Arena);

    u8 *NullTerminatedPath = PushCString(Scratch.Arena, Path);

    u64 BufferSize = PATH_MAX+1;
    u8 *Buffer = PushArrayNoZero(Arena, BufferSize, u8);
    u8 *AbsolutePath = (u8 *)realpath((char *)NullTerminatedPath, (char *)Buffer);
    if (AbsolutePath) {
        u64 Length = StringLength(Buffer);
        // TODO: End with '/' if directory
        //Buffer[Length] = '/';
        //Length += 1;
        Result = CreateString(Buffer, Length);
        PopSize(Arena, BufferSize - Length);
    }

    ReleaseScratch(Scratch);

    return Result;
}

os_directory_iterator OS_DirectoryIterator(string DirectoryPath) {
    temp_arena Scratch = GetScratch();

    u8 *NullTerminatedDirectoryPath = PushCString(Scratch.Arena, DirectoryPath);
    DIR *Dir = opendir((char *)NullTerminatedDirectoryPath);
    os_directory_iterator Iter = {
        .Handle = (void *)Dir,
    };

    ReleaseScratch(Scratch);
    return Iter;
}

void OS_DirectoryIteratorClose(os_directory_iterator *Iter) {
    closedir((DIR *)Iter->Handle);
    *Iter = {};
}

bool OS_DirectoryIteratorIsOk(os_directory_iterator Iter) {
    return Iter.Handle != 0;
}

os_filesystem_entry OS_DirectoryIteratorNext(arena *Arena, os_directory_iterator *Iter) {
    os_filesystem_entry FilesystemEntry = {};
    // TODO: This isn't very good, probably want to separate error and empty directory cases more clearly.
    FilesystemEntry.Info = OS_FileInfo_Error;

    for (struct dirent *Entry = readdir((DIR *)Iter->Handle);
         Entry != 0;
         Entry = readdir((DIR *)Iter->Handle)) {
        if ((Entry->d_name[0] == '.' && Entry->d_name[1] == 0) ||
            (Entry->d_name[0] == '.' && Entry->d_name[1] == '.' && Entry->d_name[2] == 0)) {
            continue;
        }

        if (Entry->d_type == DT_DIR) {
            FilesystemEntry.Info = OS_FileInfo_Directory;
            FilesystemEntry.Name = PushStringf(Arena, "%s/", Entry->d_name);
        }
        else {
            FilesystemEntry.Info = OS_FileInfo_File;
            FilesystemEntry.Name = PushString(Arena, CreateString((u8 *)Entry->d_name));
        }

        break;
    }

    return FilesystemEntry;
}

string OS_ReadEntireFile(arena *Arena, string FilePath)
{
    string Contents = {};
    
    temp_arena Scratch = GetScratch(Arena);
    
    u8 *PathZ  = PushCString(Scratch.Arena, FilePath);
    int FileHandle = open((char *)PathZ, O_RDONLY);
    if (FileHandle >= 0)
    {
        struct stat64 Info = {};
        if (fstat64(FileHandle, &Info) == 0)
        {
            Assert(S_ISREG(Info.st_mode));
            u64 FileSize = Info.st_size;
            u8 *Buffer = PushArrayNoZero(Arena, FileSize, u8);
            Assert(FileSize <= S64Max);
            
            ssize_t ReadCount = read(FileHandle, Buffer, FileSize);
            if (ReadCount == FileSize)
            {
                Contents.Str = Buffer;
                Contents.Length = FileSize;
            }
            else
            {
                PopSize(Arena, FileSize);
            }
        }
        
        close(FileHandle);
    }
    
    ReleaseScratch(Scratch);
    
    return Contents;
}

bool OS_WriteEntireFile(u64 Size, u8 *Contents, string FilePath)
{
    Assert(Size <= S64Max);
    
    bool Success = false;
    
    temp_arena Scratch = GetScratch();
    
    u8 *PathZ  = PushCString(Scratch.Arena, FilePath);
    int FileHandle = creat((char *)PathZ, 0);
    if (FileHandle >= 0)
    {
        ssize_t WriteCount = write(FileHandle, Contents, Size);
        if (WriteCount == Size)
        {
            Success = true;
        }
        
        close(FileHandle);
    }
    ReleaseScratch(Scratch);
    
    return Success;
}

bool OS_DeleteFile(string FilePath)
{
    temp_arena Scratch = GetScratch();
    
    u8 *PathZ  = PushCString(Scratch.Arena, FilePath);
    bool Success = (unlink((char *)PathZ) == 0);
    
    ReleaseScratch(Scratch);
    
    return Success;
}

void OS_CloseFile(os_file_handle Handle) 
{
    close((int)Handle);
}


///////////////////////////////////////////////////////////////////////
// Memory
//

// All this memory stuff still relies on overcommit.

void *OS_MemoryReserve(u64 Size)
{
    Assert(Size > 0);
    Assert(sysconf(_SC_PAGE_SIZE) == 4096);
    
    void *Memory = mmap(0, Size, PROT_NONE, MAP_ANONYMOUS|MAP_PRIVATE|MAP_NORESERVE, -1, 0);
    if (Memory == MAP_FAILED)
    {
        Memory = 0;
    }
    
    return Memory;
}

// I don't know if this is correct
bool OS_MemoryCommit(void *Address, u64 Size)
{
    Assert(Address);
    Assert(Size > 0);
    OS_MemoryRemoveGuard(Address, Size);
    // Ideally we would zero the memory here to make similar to windows, but this doesn't work when using 
    // AddressSanitizer as we haven't unpoisoned our memory yet. But if we are using address sanitizer we
    // don't have much to worry about.
    return true; 
}

// I don't know if this is correct
void OS_MemoryDecommit(void *Address, u64 Size)
{
    Assert(Address);
    Assert(Size > 0);
    
    OS_MemoryGuard(Address, Size);
    // MADV_DONTNEED
    // Do not expect access in the near future. (For the time being, the application is finished with the given range, so the kernel can free resources associated with it.) Subsequent accesses of pages in this range will succeed, but will result either in reloading of the memory contents from the underlying mapped file (see mmap(2)) or zero-fill-on-demand pages for mappings without an underlying file. 
    madvise(Address, Size, MADV_DONTNEED); 
}

void OS_MemoryFree(void *Address, u64 Size)
{
    Assert(Address);
    Assert(Size > 0);
    munmap(Address, Size);
}

void OS_MemoryGuard(void *Address, u64 Size)
{
    Assert(Address);
    Assert(Size > 0);
    Assert(((uintptr_t)Address & (4096 - 1)) == 0);
    Assert((Size & (4096 - 1)) == 0);
    
    mprotect(Address, Size, PROT_NONE);
}

void OS_MemoryRemoveGuard(void *Address, u64 Size)
{
    Assert(Address);
    Assert(Size > 0);
    Assert(((uintptr_t)Address & (4096 - 1)) == 0);
    Assert((Size & (4096 - 1)) == 0);
    
    mprotect(Address, Size, PROT_READ|PROT_WRITE);
}

u64 OS_GetWallClockMicroseconds()
{
    // We could just return nanoseconds, which can fix another 584 years in a u64, 
    // then we wouldn't need to divide nanoseconds.
    timespec Now = {};
    clock_gettime(CLOCK_MONOTONIC, &Now);
    u64 Time = (Now.tv_sec * 1000) + (Now.tv_nsec / 1000000);
    return Time;
}
