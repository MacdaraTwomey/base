
#include "base.h"
#include "base.cpp"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl>

bool PlatformFileExists(string FilePath)
{
    temp_memory Scratch = GetScratch();
    u8 *PathZ  = PushCString(&Scratch, FilePath);
    
    // F_OK checks for existence
    int Result = access(PathZ, F_OK);
    
    ReleaseScratch(Scratch);
    
    return (Result == 0);
}

u64 PlatformGetFileSize(string FilePath)
{
    temp_memory Scratch = GetScratch();
    u8 *PathZ  = PushCString(&Scratch, FilePath);
    
    u64 Size = 0;
    
    // stat() will follow links so the stat()'ed file should never be a symbolic link.
    // It could be a directory, pipe, socket, character device, fifo though.
    struct stat64 Info = {};
    if (stat64(PathZ, &Info) == 0)
    {
        Assert(S_ISREG(Info.st_mode));
        Size = st_size;
    }
    
    ReleaseScratch(Scratch);
    
    return Size;
}

string PlatformGetExecutablePath(arena *Arena)
{
    string Path = {};
    
    u64 BufferSize = KB(8);
    u8 *Buffer = PushArray(Arena, BufferSize, u8);
    ssize_t PathLength = readlink("/proc/self/exe", Buffer, BufferSize);
    if (PathLength != -1)
    {
        u64 Excess = BufferSize - PathLength;
        PopSize(Arena, Excess);
        Path = CreateString(Buffer, PathLength);
        break;
    }
    
    return Path;
}

platform_file_contents PlatformReadEntireFile(arena *Arena, string FilePath)
{
    platform_file_contents Contents = {};
    
    temp_memory Scratch = GetScratch();
    
    u8 *PathZ  = PushCString(&Scratch, FilePath);
    int FileHandle = open(PathZ, O_RDONLY);
    if (FileHandle >= 0)
    {
        struct stat64 Info = {};
        if (fstat64(FileHandle, &Info) == 0)
        {
            Assert(IS_REG(Info.st_mode));
            u64 FileSize = st_size;
            u8 *Buffer = PushArray(Arena, FileSize, u8);
            Assert(FileSize <= SSIZE_MAX);
            
            ssize_t ReadCount = read(FileHandle, Buffer, FileSize);
            if (ReadCount == FileSize)
            {
                Contents.Contents = Buffer;
                Contents.Size = FileSize;
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
    Assert(Size <= SSIZE_MAX);
    
    bool Success = false;
    
    temp_memory Scratch = GetScratch();
    
    u8 *PathZ  = PushCString(&Scratch, FilePath);
    int FileHandle = open(PathZ, O_WRONLY);
    if (FileHandle >= 0)
    {
        ssize_t WriteCount = write(FileHandle, Size, Contents);
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
    temp_memory Scratch = GetScratch();
    
    u8 *PathZ  = PushCString(&Scratch, FilePath);
    bool Success = (unlink(PathZ) == 0);
    
    ReleaseScratch(Scratch);
    
    return Success;
}

///////////////////////////////////////////////////////////////////////
// Memory
//

void *  PlatformMemoryReserve(u64 Size);
bool    PlatformMemoryCommit(void *Address, u64 size);
void    PlatformMemoryDecommit(void *Address, u64 Size);
void    PlatformMemoryFree(void *Address);
void    PlatformMemoryGuard(void *Address, u64 Size);
void    PlatformMemoryRemoveGuard(void *Address, u64 Size);


int main(int ArgCount, char *Args[]) 
{
    
}