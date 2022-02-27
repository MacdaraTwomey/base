
#include "base.h"
#include "base.cpp"
#include "platform.h"

#include <windows.h>

static LARGE_INTEGER GlobalPerformanceFrequency = {};

//
// TODO:
// - PLATFORM_ERROR_MESSAGE to make message box
// - Our own custom utf8 conversion code

HANDLE Win32CreateConsole()
{
    AllocConsole();
    
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONIN", "r", stdin);
    
    HANDLE con_out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD out_mode;
    GetConsoleMode(con_out, &out_mode);
    
    return GetStdHandle(STD_INPUT_HANDLE);
}

s64 Win32GetTime()
{
    LARGE_INTEGER Now;
    QueryPerformanceCounter(&Now);
    return Now.QuadPart;
}

s64 Win32GetMicrosecondsElaped(s64 start, s64 end)
{
    s64 Microseconds;
    Microseconds = end - start;
    Microseconds *= 1000000; // microseconds per second
    Microseconds /= GlobalPerformanceFrequency.QuadPart; // ticks per second
    return Microseconds;
}

// Returns non-null terminated string
string UTF8FromUTF16(arena *Arena, wchar_t *String16, u32 String16Length)
{
    Assert(String16Length > 0);
    
    int Capacity = (String16Length * 4);
    u8 *Buffer = Allocate(Arena, Capacity, u8);
    
    // Returns 0 on failure
    int ByteCount = WideCharToMultiByte(CP_UTF8, 0, 
                                        String16, String16Length, 
                                        (char *)Buffer, Capacity, 
                                        NULL, NULL);
    PopSize(Arena, Capacity - ByteCount);
    return MakeString(Buffer, ByteCount);
}

// Returns null-terminated string
wchar_t *UTF16FromUTF8(arena *Arena, u8 *String8, int String8Length)
{
    Assert(String8Length > 0);
    
    int Capacity = (String8Length * 2) + 2;
    wchar_t *Buffer = Allocate(Arena, Capacity, wchar_t);
    
    int CharacterCount = MultiByteToWideChar(CP_UTF8, 0, 
                                             (char *)String8, String8Length, 
                                             Buffer, Capacity);
    Buffer[CharacterCount] = 0;
    return Buffer;
}

// TODO: May be prefixed with \? stuff... test this
string PlatformGetExecutablePath(arena *Arena)
{
    temp_memory Scratch = GetScratch();
    
    DWORD Capacity = 2048;
    wchar_t *Buffer = Allocate(Scratch.Arena, Capacity, wchar_t);
    
    u32 Size = 0;
    for (int Tries = 0; Tries < 4; ++Tries)
    {
        Size = GetModuleFileNameW(NULL, Buffer, Capacity);
        if (Size == Capacity && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
        {
            // String truncated to Size characters (including null terminator)
            // Try again with larger Buffer
            Capacity *= 2;
            Buffer = Allocate(Scratch.Arena, Capacity, wchar_t);
        }
        else
        {
            // Error or Success
            break;
        }
    }
    
    // If succeeded then Size is the length of the Path (NOT including the null terminator). 
    // If failed then Size is 0.
    // Or if we failed 4 times Size will == Capacity (TODO: What happens if size is capacity but wan't truncated)?
    // will GetLastError just not be set and we can use path.
    
    // From docs:
    // "If the length of the path is less than the size that the nSize parameter specifies, the function succeeds and the path is returned as a null-terminated string."
    // "And if exceeds then truncates..."
    
    string Result = {};
    if (Size > 0)
    {
        Result = UTF8FromUTF16(Arena, Buffer, Size);
    }
    
    ReleaseScratch(Scratch);
    
    return Result;
}

bool PlatformFileExists(string FileName)
{
    // I think you need to prepend \\?\ to exceed 260 Characters
    // TODO: Test with a long path
    // This null terminates for us
    temp_memory Scratch = GetScratch();
    
    wchar_t *FileName16 = UTF16FromUTF8(Scratch.Arena, FileName.Str, (int)FileName.Length);
    DWORD Result = GetFileAttributesW(FileName16);
    bool IsDirectory = Result & FILE_ATTRIBUTE_DIRECTORY;
    bool Exists = Result != INVALID_FILE_ATTRIBUTES; 
    
    ReleaseScratch(Scratch);
    
    return Exists && !IsDirectory;
}

u64 PlatformGetFileSize(string FileName)
{
    u64 Result = 0;
    
    temp_memory Scratch = GetScratch();
    
    wchar_t *FileName16 = UTF16FromUTF8(Scratch.Arena, FileName.Str, (int)FileName.Length);
    
    HANDLE File = CreateFileW(FileName16, GENERIC_READ, FILE_SHARE_READ, NULL, 
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (File != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER Size;
        if (GetFileSizeEx(File, &Size))
        {
            Result = Size.QuadPart;
        }
    }
    
    ReleaseScratch(Scratch);
    
    return Result;
}

platform_file_contents PlatformReadEntireFile(arena *Arena, string FileName)
{
    platform_file_contents Result = {};
    
    temp_memory Scratch = GetScratch();
    
    wchar_t *FileName16 = UTF16FromUTF8(Scratch.Arena, FileName.Str, (int)FileName.Length);
    
    HANDLE File = CreateFileW(FileName16, GENERIC_READ, FILE_SHARE_READ, NULL, 
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (File != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER Size;
        if (GetFileSizeEx(File, &Size))
        {
            u64 FileSize = Size.QuadPart;
            if (FileSize > 0)
            {
                u8 *Contents = Allocate(Arena, FileSize, u8);
                if (Contents)
                {
                    u8 *Dest = Contents;
                    u64 BytesRemaining = FileSize;
                    while (BytesRemaining > 0)
                    {
                        DWORD ChunkSize = (DWORD)ClampTop(BytesRemaining, UINT32_MAX);
                        DWORD BytesRead = 0;
                        if (ReadFile(File, Dest, ChunkSize, &BytesRead, 0))
                        {
                            BytesRemaining -= BytesRead;
                            Dest += BytesRead;
                        }
                        else
                        {
                            break;
                        }
                    }
                    
                    if (BytesRemaining == 0)
                    {
                        Result.Contents = Contents;
                        Result.Size = FileSize;
                    }
                    else
                    {
                        PopSize(Arena, FileSize);
                    }
                }
            }
        }
        
        CloseHandle(File);
    }
    
    ReleaseScratch(Scratch);
    
    return Result;
}

void *PlatformMemoryReserve(u64 Size)
{
    Assert(Size > 0);
    void *Memory = VirtualAlloc(0, Size, MEM_RESERVE, PAGE_NOACCESS);
    return Memory;
}

bool PlatformMemoryCommit(void *Address, u64 Size)
{
    // Can succeed even if Address is NULL
    // Does not fail if you try to commit an already commited page
    // Actual physical pages are not allocated until they are accessed
    // Zeros the commited pages
    Assert(Address);
    Assert(Size > 0);
    bool Success = (VirtualAlloc(Address, Size, MEM_COMMIT, PAGE_READWRITE) != 0);
    return Success;
}

void PlatformMemoryDecommit(void *Address, u64 Size)
{
    // Frees all pages with a byte in the range from Address to Address+Size
    Assert(Address);
    Assert(Size > 0);
    // Does not fail if you try to decommit an uncommited page
    VirtualFree(Address, Size, MEM_DECOMMIT);
}

void PlatformMemoryFree(void *Address)
{
    // Decommits all commited pages and releases reserved memory region
    Assert(Address);
    VirtualFree(Address, 0, MEM_RELEASE);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)pCmdLine;
    (void)nCmdShow;
    
    {
        arena *Arena = CreateArena(GB(300));
        
        u32 *Array1 = Allocate(Arena, MB(30) / 4, u32);
        u32 *Array2 = Allocate(Arena, MB(30) / 4, u32, NoClear());
        
        for (u32 i = 0; i < 10000; ++i)
        {
            Array1[i] = i;
            Array2[i] = i * 2;
        }
        
        MemoryZero(sizeof(u32) * 10000, Array1);
        MemoryCopy(sizeof(u32) * 10000, Array1, Array2);
        MemoryCopyOverlapped(sizeof(u32) * 10000, Array1, Array2);
        
        u64 Pos = Arena->Pos;
        
        f32 *F32Array1 = (f32 *)PushSize_(Arena,             MB(40), 4, NoClear());
        f32 *F32Array2 = (f32 *)PushCopy_(Arena,  F32Array1, MB(40), 4);
        f32 *F32Array3 =        PushCopy(Arena,  F32Array1, MB(40), f32);
        MemoryZero(MB(40), F32Array1);
        MemoryZero(MB(40), F32Array2);
        MemoryZero(sizeof(f32) * MB(40), F32Array3);
        
        PopToPosition(Arena, Pos);
        
        u8 *Array3 = Allocate(Arena, MB(100), u8);
        
        for (u32 i = 0; i < MB(100); ++i)
            Array3[i] = (u8)i;
        
        temp_memory TempMemory1 = BeginTempMemory(Arena);
        u8 *Array4 = Allocate(Arena, 100, u8);
        MemoryZero(100, Array4);
        temp_memory TempMemory2 = BeginTempMemory(Arena);
        u8 *Array5 = Allocate(Arena, 100, u8);
        MemoryZero(100, Array5);
        
        EndTempMemory(TempMemory1); // In wrong order
        EndTempMemory(TempMemory2);
        
        temp_memory Scratch = GetScratch();
        
        u16 *Array6 = Allocate(Scratch.Arena, 10000, u16);
        MemoryZero(10000 * 2, Array6);
        arena *Struct = PushStruct(Scratch.Arena, arena);
        Struct->Base = 0;
        Struct->TempCount = 239487239;
        
        ReleaseScratch(Scratch);
        
        // Should this be allowed?
        //u32 *ZeroByteAlloc = Allocate(Arena, 0, u32);
        //Assert(ZeroByteAlloc != 0);
        
        FreeArena(Arena);
    }
    
    {
        arena *Arena = CreateArena(GB(1));
        
        const char *conststr = "0123456789";
        string NumberString = PushFormatString(Arena, (char *)"%s %u ABCDEF, !%f", conststr, 2342u, 0.0002342234f);
        
        string StringUpper = StringLit("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        string StringLower = StringLit("abcdefghijklmnopqrstuvwxyz");
        string StringNumbers = StringLit("0123456789");
        
        u8 C1 = StringGetChar(StringUpper, 25);
        Assert(C1 == 'Z');
        u8 C2 = StringGetChar(StringUpper, 26);
        Assert(C2 == 0);
        
        for (u32 i = 0; i < 26; ++i)
        {
            Assert(IsUpper(StringUpper[i]));
            Assert(IsLower(StringLower[i]));
            Assert(IsAlpha(StringUpper[i]) && IsAlpha(StringLower[i]));
        }
        
        for (u32 i = 0; i < 10; ++i)
        {
            Assert(IsNumeric(StringNumbers[i]));
        }
        
        u64 Index1 = StringFindChar(StringUpper, 'Y', 10);
        Assert(Index1 == 24);
        
        u64 Index2 = StringFindChar(StringUpper, '7');
        Assert(Index2 == StringUpper.Length);
        
        u64 Index3 = StringFindCharReverse(StringUpper, 'D');
        Assert(Index3 == 3);
        
        u64 Index4 = StringFindCharReverse(StringLit(""), '6');
        Assert(Index4 == 0);
        
        Assert(StringContainsChar(StringLit("ABC"), 'B'));
        Assert(StringContainsChar(StringLit("B"), 'B'));
        Assert(!StringContainsChar(StringLit(""), 'B'));
        
        Assert(StringContainsSubstr(StringLit("abcdefghij"), StringLit("defg")));
        Assert(!StringContainsSubstr(StringLit("abc"), StringLit("abcd")));
        Assert(!StringContainsSubstr(StringLit(""), StringLit("abcd")));
        Assert(!StringContainsSubstr(StringLit("abc"), StringLit("")));
        
        //                                               01234567890123456789
        u64 SlashIndex1 = StringFindLastSlash(StringLit("c:/dev/test/file.exe"));
        Assert(SlashIndex1 == 11);
        
        string FileExe = StringLit("file.exe");
        u64 SlashIndex2 = StringFindLastSlash(FileExe);
        Assert(SlashIndex2 == FileExe.Length);
        
        string TestDotTarGz = StringLit("test.tar.gz");
        string TestDotObjDot = StringLit("test.obj.");
        string TestDotDotObj = StringLit("test..obj");
        string Test = StringLit("test");
        string Dot = StringLit(".");
        StringRemoveExtension(&FileExe);
        StringRemoveExtension(&TestDotTarGz);
        StringRemoveExtension(&TestDotObjDot);
        StringRemoveExtension(&TestDotDotObj);
        StringRemoveExtension(&Test);
        StringRemoveExtension(&Dot);
        
        Assert(StringsAreEqual(FileExe, StringLit("file"))); 
        Assert(StringsAreEqual(TestDotTarGz, StringLit("test.tar"))); 
        Assert(StringsAreEqual(TestDotObjDot, StringLit("test.obj"))); 
        Assert(StringsAreEqual(TestDotDotObj, StringLit("test."))); 
        Assert(StringsAreEqual(Test, StringLit("test"))); 
        Assert(StringsAreEqual(Dot, StringLit(""))); 
        
        Assert(StringsAreEqual(StringLit("file.exe"), StringFilenameFromPath(StringLit("c:/dev/test/file.exe")))); 
        Assert(StringsAreEqual(StringLit("file.exe"), StringFilenameFromPath(StringLit("c://dev/test////file.exe")))); 
        Assert(StringsAreEqual(StringLit("file.exe"), StringFilenameFromPath(StringLit("/c/dev/test/file.exe")))); 
        Assert(StringsAreEqual(StringLit("file"), StringFilenameFromPath(StringLit("/c/dev/test/file")))); 
        Assert(StringsAreEqual(StringLit("file"), StringFilenameFromPath(StringLit("/c/.dev/test //file")))); 
        Assert(StringsAreEqual(StringLit(""), StringFilenameFromPath(StringLit("/c/dev/test/file/")))); 
        Assert(StringsAreEqual(StringLit("file"), StringFilenameFromPath(StringLit("file")))); 
        Assert(StringsAreEqual(StringLit(""), StringFilenameFromPath(StringLit("/")))); 
        Assert(StringsAreEqual(StringLit("c"), StringFilenameFromPath(StringLit("/c")))); 
        Assert(StringsAreEqual(StringLit("elmo.doc"), StringFilenameFromPath(StringLit("c:/abc\\def//hjkl\\\\sdfsdf/elmo.doc")))); 
        
        temp_memory Scratch = GetScratch();
        
        string_builder Builder = CreateStringBuilder();
        Builder.Append(Scratch.Arena, (char *)"This is the first string. ");
        Builder.Append(Scratch.Arena, StringLit("This is the 2nd string."));
        Builder.Append(Scratch.Arena, StringLit("The last string."));
        Builder.Append(Scratch.Arena, StringLit("\0"));
        
        string WholeString = Builder.GetString(Arena);
        
        ReleaseScratch(Scratch);
        
        FreeArena(Arena);
    }
    
    arena *Arena = CreateArena(GB(10));
    
    string String8 = UTF8FromUTF16(Arena, (wchar_t *)L"Hellow", 6);
    wchar_t *String16 = UTF16FromUTF8(Arena, (u8 *)"Hellow", 6);
    (void)String16;
    
    string ExePath = PlatformGetExecutablePath(Arena);
    Assert(PlatformFileExists(StringLit("c:/dev/base/build/test.md")));
    Assert(PlatformFileExists(StringLit("c:/dev/base/build/base_test.exe")));
    Assert(PlatformFileExists(StringLit("c:\\dev\\base\\build\\base_test.exe")));
    Assert(PlatformFileExists(StringLit("c:\\dev\\base\\build\\BASE_TEST.EXE")));
    Assert(!PlatformFileExists(StringLit("c:\\dev\\base\\build\\")));
    Assert(!PlatformFileExists(StringLit("c:\\dev\\base\\build")));
    Assert(!PlatformFileExists(StringLit("c:\\fake\\fakey")));
    
    
    Assert(PlatformGetFileSize(StringLit("c:\\dev\\base\\build\\test.md")) == 815);
    
    platform_file_contents File = PlatformReadEntireFile(Arena, StringLit("test.md"));
    
#if 0
    u64 TotalSize = GB(5);
    u64 ChunkSize = TotalSize / 32;
    Assert(TotalSize % 32 == 0);
    u32 WriteCount = 32;
    u8 *Chunk = Allocate(Arena, ChunkSize, u8);
    
    FILE *fp = fopen("large.big", "wb");
    for (u32 i = 0; i < WriteCount; ++i)
    {
        fwrite(Chunk, ChunkSize, 1, fp);
    }
    fclose(fp);
    
    PopSize(Arena, ChunkSize);
    
    platform_file_contents LargeFile = PlatformReadEntireFile(Arena, StringLit("large.big"));
    
    DeleteFile("large.big");
#endif
    
    FreeArena(Arena);
    
    return 0;
}

