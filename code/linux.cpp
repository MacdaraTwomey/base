
#include "platform.h"
#include "base.h"
#include "base.cpp"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>

// TODO: Maybe allow direcotries to return true
// If we do this, then we can use access().
bool PlatformFileExists(string FilePath)
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

u64 PlatformGetFileSize(string FilePath)
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

string PlatformGetExecutablePath(arena *Arena)
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

string PlatformReadEntireFile(arena *Arena, string FilePath)
{
    string Contents = {};
    
    temp_arena Scratch = GetScratch();
    
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

bool PlatformWriteEntireFile(u64 Size, u8 *Contents, string FilePath)
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

bool PlatformDeleteFile(string FilePath)
{
    temp_arena Scratch = GetScratch();
    
    u8 *PathZ  = PushCString(Scratch.Arena, FilePath);
    bool Success = (unlink((char *)PathZ) == 0);
    
    ReleaseScratch(Scratch);
    
    return Success;
}

///////////////////////////////////////////////////////////////////////
// Memory
//

// All this memory stuff still relies on overcommit.

void *PlatformMemoryReserve(u64 Size)
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
bool PlatformMemoryCommit(void *Address, u64 Size)
{
    Assert(Address);
    Assert(Size > 0);
    PlatformMemoryRemoveGuard(Address, Size);
    MemoryZero(Size, Address);
    return true; 
}

// I don't know if this is correct
void PlatformMemoryDecommit(void *Address, u64 Size)
{
    Assert(Address);
    Assert(Size > 0);
    
    PlatformMemoryGuard(Address, Size);
    // MADV_DONTNEED
    // Do not expect access in the near future. (For the time being, the application is finished with the given range, so the kernel can free resources associated with it.) Subsequent accesses of pages in this range will succeed, but will result either in reloading of the memory contents from the underlying mapped file (see mmap(2)) or zero-fill-on-demand pages for mappings without an underlying file. 
    madvise(Address, Size, MADV_DONTNEED); 
}

void PlatformMemoryFree(void *Address, u64 Size)
{
    Assert(Address);
    Assert(Size > 0);
    munmap(Address, Size);
}

void PlatformMemoryGuard(void *Address, u64 Size)
{
    Assert(Address);
    Assert(Size > 0);
    Assert(((uintptr_t)Address & (4096 - 1)) == 0);
    Assert((Size & (4096 - 1)) == 0);
    
    mprotect(Address, Size, PROT_NONE);
}

void PlatformMemoryRemoveGuard(void *Address, u64 Size)
{
    Assert(Address);
    Assert(Size > 0);
    Assert(((uintptr_t)Address & (4096 - 1)) == 0);
    Assert((Size & (4096 - 1)) == 0);
    
    mprotect(Address, Size, PROT_READ|PROT_WRITE);
}

#include "test.cpp"

int main(int ArgCount, char *Args[]) 
{
    RunTests();
}