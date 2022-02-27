
#include <stdarg.h>
#include <stdio.h> // vsnprintf,
#include <string.h> // memset, memcpy, memmove

#include "platform.h"
#include "base.h"

///////////////////////////////////////////////////////////////////////
// Arena
//

// TODO:
// Guard pages? via win32 VirtualProtect
// Commit minimum block size, then don't need to pass in an initial commit size
// 

// TODO: Make thread local
// TODO: Do we wan't a pool of possible scratch arenas
static arena *GlobalScratchArena_ = nullptr;

#define ARENA_HEADER_SIZE 64
#define PAGE_SIZE 4096
#define COMMIT_CHUNK_SIZE MB(32)

static_assert(sizeof(arena) <= ARENA_HEADER_SIZE, "Must be able to fit arena struct into start of arena memory");

void MemoryCopy(u64 Size, void *Source, void *Dest)
{
    Assert(Source != nullptr && Dest != nullptr);
    memcpy(Dest, Source, Size);
}

void MemoryCopyOverlapped(u64 Size, void *Source, void *Dest)
{
    Assert(Source != nullptr && Dest != nullptr);
    memmove(Dest, Source, Size);
}

void MemoryZero(u64 Size, void *Memory)
{
    Assert(Memory != nullptr);
    memset(Memory, 0, Size);
}

bool IsPowerOfTwo(u64 x) 
{
    return (x & (x-1)) == 0;
}

u64 AlignUp(u64 Value, u64 Align)
{
    Assert(IsPowerOfTwo(Align));
    
    // Add maximum alignment amount (align - 1)
    // Then round off what we don't need by applying mask
    u64 AlignMask = Align - 1;
    return (Value + AlignMask) & ~AlignMask;
}

arena *CreateArena(u64 ReserveSize)
{
    Assert(ReserveSize > ARENA_HEADER_SIZE);
    
    arena *Arena = nullptr;
    
    // On windows 8 and above it is very cheap to reserve memory, you only start using page tables
    // when you commit the pages.
    // On linux page tables are only used when the memory is touched because of overcommit.
    u64 AlignedReserveSize = AlignUp(ReserveSize, PAGE_SIZE);
    void *Memory = PlatformMemoryReserve(AlignedReserveSize);
    if (Memory)
    {
        u32 CommitSize = PAGE_SIZE;
        if (PlatformMemoryCommit(Memory, CommitSize))
        {
            Arena = (arena *)Memory;
            
            Arena->Base = (u8 *)Memory;
            Arena->Pos = ARENA_HEADER_SIZE;
            Arena->Commit = CommitSize;
            Arena->Reserve = ReserveSize;
            Arena->TempCount = 0;
        }
        else
        {
            PlatformMemoryFree(Memory);
        }
    }
    
    Assert(Arena);
    
    return Arena;
}

void FreeArena(arena *Arena)
{
    PlatformMemoryFree(Arena->Base);
}

void *PushSize_(arena *Arena, u64 Size, u32 Alignment, u8 ArenaPushFlags)
{
    // It is not really necessary to properly align allocations on modern x64 processors (unless you are doing SIMD, or possibly on ARM), 
    // It may be undefined behaviour to access an unaligned type, but we are already ignoring strict aliasing.
    
    Assert(Size > 0);
    
    u8 *Result = nullptr;
    u64 AlignedPos = AlignUp(Arena->Pos, Alignment);
    u64 NewPos = AlignedPos + Size; 
    if (NewPos < Arena->Reserve)
    {
        if (NewPos > Arena->Commit)
        {
            u64 TotalCommitSize = AlignUp(NewPos, COMMIT_CHUNK_SIZE);
            u64 NewCommit = ClampTop(TotalCommitSize, Arena->Reserve);
            u64 NewCommitSize = NewCommit - Arena->Commit;
            if (PlatformMemoryCommit(Arena->Base + Arena->Commit, NewCommitSize))
            {
                // Unfortunately we still need to zero the memory that is before the newly commited memory, otherwise we would rely on the platform commit and disable the ClearToZero zero flag.
                Arena->Commit = NewCommit;
                Assert(Arena->Commit % PAGE_SIZE == 0);
            }
        }
        
        if (NewPos <= Arena->Commit)
        {
            Result = Arena->Base + AlignedPos;
            Arena->Pos = NewPos;
            
            if (ArenaPushFlags & ArenaPushFlags_ClearToZero)
            {
                MemoryZero(Size, Result);
            }
        }
    }
    
    // TODO: Could return pointer to stub of preallocated memory on failure then set some arena error flag
    Assert(Result);
    
    return Result;
}

void *PushCopy_(arena *Arena, void *Source, u64 Size, u32 Alignment)
{
    void *Memory = PushSize_(Arena, Size, Alignment, ArenaPushFlags_None);
    MemoryCopy(Size, Source, Memory);
    return Memory;
}

void PopToPosition(arena *Arena, u64 Position)
{
    // Prevent popping arena struct at start of allocated memory
    // or if the position is above the current arena pos.
    Assert(Position >= ARENA_HEADER_SIZE);
    
    u64 NewPos = Clamp(Position, (u64)ARENA_HEADER_SIZE, Arena->Pos);
    
    // Ensure that there is at most a commit chunk free
    if (NewPos + COMMIT_CHUNK_SIZE < Arena->Commit)
    {
        u64 NewCommit = AlignUp(Arena->Commit - (NewPos + COMMIT_CHUNK_SIZE), PAGE_SIZE);
        u64 DecommitSize = Arena->Commit - NewCommit;
        PlatformMemoryDecommit(Arena->Base + NewCommit, DecommitSize);
        Arena->Commit = NewCommit;
    }
    
    Arena->Pos = NewPos;
}

void PopSize(arena *Arena, u64 Size)
{
    Assert(Arena->Pos > Size);
    PopToPosition(Arena, Arena->Pos - Size);
}

temp_memory BeginTempMemory(arena *Arena)
{
    temp_memory Temp = {};
    Temp.Arena = Arena;
    Temp.StartPos = Arena->Pos; 
    
    Arena->TempCount += 1;
    
    return Temp;
}

void EndTempMemory(temp_memory TempMemory)
{
    arena *Arena = TempMemory.Arena;
    Assert(Arena->TempCount > 0);
    
    PopToPosition(Arena, TempMemory.StartPos);
    Arena->TempCount -= 1;
}

void CheckArena(arena *Arena)
{
    Assert(Arena->TempCount == 0);
}

temp_memory GetScratch()
{
    if (GlobalScratchArena_ == nullptr)
    {
        GlobalScratchArena_ = CreateArena(GB(8));
    }
    
    arena *Arena = GlobalScratchArena_;
    
    Assert(Arena->TempCount == 0);
    return BeginTempMemory(Arena);
}

void ReleaseScratch(temp_memory ScratchMemory)
{
    Assert(ScratchMemory.Arena == GlobalScratchArena_);
    Assert(ScratchMemory.Arena->TempCount == 1);
    EndTempMemory(ScratchMemory);
} 

///////////////////////////////////////////////////////////////////////
// String
//

u64 StringLength(u8 *CString)
{
    u64 Length = 0; 
    while (*CString) 
    {
        ++CString;
        Length += 1;
    }
    
    return Length;
}

string MakeString(u8 *CString)
{
    string Result;
    Result.Str = CString;
    Result.Length = StringLength(CString);
    return Result;
}

string MakeString(u8 *StringData, u64 Length)
{
    string Result;
    Result.Str = StringData;
    Result.Length = Length;
    return Result;
}

// Null terminator included
u8 *PushCString(arena *Arena, u8 *String)
{
    u64 Length = StringLength(String) + 1;
    u8 *Memory = (u8 *)PushCopy_(Arena, String, Length, 1);
    return Memory;
}

string PushString(arena *Arena, string String)
{
    u8 *StringData = (u8 *)PushCopy_(Arena, String.Str, String.Length, 1);
    return MakeString(StringData, String.Length);
}

void BaseAssertPrint_(const char *Format, ...)
{
    // We already include stdio in base.cpp to get vsnprintf so might as well use it here for printing to console
    va_list Args;
    va_start (Args, Format);
    vprintf(Format, Args);
    va_end (Args);
    fflush(stdout);
}

string PushFormatStringArgs(arena *Arena, char *Format, va_list Args)
{
    u64 Length = 0;
    u64 Capacity = 1024;
    u8 *Buffer = Allocate(Arena, Capacity, u8, NoClear());
    
    // We have to copy the va_list because we may call vsnprintf twice
    va_list ArgsCopy;
    va_copy(ArgsCopy, Args);
    
    int RequiredCharCount = vsnprintf((char *)Buffer, Capacity, Format, Args);
    if (RequiredCharCount < 0)
    {
        // Error
        Length = 0;
        Assert(0);
    }
    else if (RequiredCharCount >= Capacity)
    {
        // The string was truncated.
        // Returns the number of chars that would have been written (not including the null terminator).
        // If RequiredCharCount == StringLength (not including null terminator) then the last char was not 
        // written as a null terminator must be written instead.
        
        PopSize(Arena, Capacity);
        u64 NewCapacity = RequiredCharCount + 1;
        Buffer = Allocate(Arena, NewCapacity, u8);
        
        int WrittenCharCount = vsnprintf((char *)Buffer, NewCapacity, Format, ArgsCopy);
        if (WrittenCharCount < 0)
        {
            Length = 0;
            Assert(0);
        }
        else
        {
            PopSize(Arena, NewCapacity - WrittenCharCount); // TODO: Is it necessary to pop one byte?
            Length = WrittenCharCount;
        }
    }
    else
    {
        Length = RequiredCharCount;
    }
    
    va_end (ArgsCopy);
    
    return MakeString(Buffer, Length);
}


string PushFormatString(arena *Arena, char *Format, ...)
{
    va_list Args;
    va_start (Args, Format);
    string Result = PushFormatStringArgs(Arena, Format, Args);
    va_end (Args);
    
    return Result;
}

u8 StringGetChar(string String, u64 Index)
{
    u8 Result = 0;
    if (Index < String.Length)
    {
        Result = String.Str[Index];
    }
    
    return Result;
}

bool IsUpper(u8 c)
{
    return (('A' <= c) && (c <= 'Z'));
}

bool IsLower(u8 c)
{
    return (('a' <= c) && (c <= 'z'));
}

bool IsAlpha(u8 c)
{
    return IsLower(c) || IsUpper(c);
}

bool IsNumeric(u8 c)
{
    return (('0' <= c) && (c <= '9'));
}

bool IsAlphaNumeric(u8 c)
{
    return IsAlpha(c) || IsNumeric(c);
}

bool IsWhitespace(u8 c)
{
    return (c == ' ' || c == '\r' || c == '\n' || c == '\t' || c == '\f' || c == '\v');
}

u8 ToUpper(u8 c)
{
    return IsLower(c) ? c - 32 : c;
}

u8 ToLower(u8 c)
{
    return IsUpper(c) ? c + 32 : c;
}

void StringToLower(string *String)
{
    for (u64 i = 0; i < String->Length; ++i)
    {
        String->Str[i] = ToLower(String->Str[i]);
    }
}

void StringToUpper(string *String)
{
    for (u64 i = 0; i < String->Length; ++i)
    {
        String->Str[i] = ToUpper(String->Str[i]);
    }
}

string StringGetPrefix(string String, u64 N)
{
    String.Length = ClampTop(N, String.Length);
    return String;
}

string StringGetSuffix(string String, u64 N)
{
    u64 SuffixLength = Minimum(N, String.Length);
    String.Str += (String.Length - SuffixLength);
    String.Length = SuffixLength;
    return String;
}

void StringSkip(string *String, u64 N)
{
    N = ClampTop(N, String->Length);
    String->Str = String->Str + N;
    String->Length -= N;
}

void StringChop(string *String, u64 N)
{
    N = ClampTop(N, String->Length);
    String->Length -= N;
}

u64 StringCountOccurence(string String, u8 Char)
{
    u64 Count = 0;
    for (u64 i = 0; i < String.Length; ++i)
    {
        if (String.Str[i] == Char)
        {
            Count += 1;
        }
    }
    
    return Count;
}

// Offest has a default value of 0
u64 StringFindChar(string String, u8 Char, u32 Offset)
{
    u64 Position = String.Length;
    for (u64 i = Offset; i < String.Length; ++i)
    {
        if (String.Str[i] == Char)
        {
            Position = i;
            break;
        }
    }
    
    return Position;
}

u64 StringFindCharReverse(string String, u8 Char)
{
    u64 Position = String.Length;
    for (u64 i = String.Length - 1; i != static_cast<u64>(-1); --i)
    {
        if (String.Str[i] == Char)
        {
            Position = i;
            break;
        }
    }
    
    return Position;
}

bool StringContainsChar(string String, u8 Char)
{
    return StringFindChar(String, Char) < String.Length;
}

bool StringsAreEqual(string A, string B)
{
    bool Result = (A.Length == B.Length);
    if (Result)
    {
        for (u64 i = 0; i < A.Length; ++i)
        {
            if (A.Str[i] != B.Str[i]) 
            {
                Result = false;
                break;
            }
        }
    }
    
    return Result;
}

bool StringsAreEqualCaseInsensitive(string a, string b)
{
    bool Result = (a.Length == b.Length);
    if (Result)
    {
        for (u64 i = 0; i < a.Length; ++i)
        {
            if (ToLower(a.Str[i]) != ToLower(b.Str[i])) 
            {
                Result = false;
                break;
            }
        }
    }
    
    return Result;
}

bool StringContainsSubstr(string String, string Substr)
{
    // O(n^2) 
    bool ContainsSubstr = false;
    if (String.Length >= Substr.Length && Substr.Length > 0) 
    {
        u64 LastPossibleCharIndex = String.Length - Substr.Length;
        for (u64 i = 0; i <= LastPossibleCharIndex; ++i)
        {
            if (String.Str[i] == Substr.Str[0])
            {
                ContainsSubstr = true;
                for (u64 j = 1; j < Substr.Length; ++j)
                {
                    if (String.Str[i + j] != Substr.Str[j])
                    {
                        ContainsSubstr = false;
                        break;
                    }
                }
                
                if (ContainsSubstr) break;
            }
        }
    }
    
    return ContainsSubstr;
}

bool CharIsSlash(u8 c)
{
    return (c == '\\' || c == '/');
}

u64 StringFindLastSlash(string Path)
{
    u64 Position = Path.Length;
    for (u64 i = Path.Length - 1; i != static_cast<u64>(-1); --i)
    {
        if (CharIsSlash(Path.Str[i]))
        {
            Position = i;
            break;
        }
    }
    
    return Position;
}

void StringRemoveExtension(string *File)
{
    // Files can have multiple dots, and only last is the real extension.
    
    // If no dot is found Length stays the same
    u64 DotPosition = StringFindCharReverse(*File, '.');
    File->Length = DotPosition;
}

string StringFilenameFromPath(string Path)
{
    u64 LastSlash = StringFindLastSlash(Path);
    
    if (LastSlash < Path.Length)
    {
        StringSkip(&Path, LastSlash + 1);
    }
    
    return Path;
}

string_builder CreateStringBuilder()
{
    string_builder Result = {};
    Result.Head = nullptr;
    Result.Tail = nullptr;
    Result.Length = 0;
    return Result;
}

void string_builder::Append(arena *Arena, string String)
{
    string_node *Node = PushStruct(Arena, string_node, 1);
    Node->String = PushString(Arena, String);
    Node->Next = nullptr;
    
    if (Head == nullptr)
    {
        Head = Node;
    }
    else
    {
        Tail->Next = Node;
    }
    
    Tail = Node;
    Length += String.Length;
}

void string_builder::Append(arena *Arena, char *String)
{
    Append(Arena, MakeString((u8 *)String));
}

string string_builder::GetString(arena *Arena)
{
    u8 *StringMemory = Allocate(Arena, Length, u8);
    
    u64 CopiedLength = 0;
    for (string_node *Node = Head; Node != nullptr; Node = Node->Next)
    {
        MemoryCopy(Node->String.Length, Node->String.Str, StringMemory + CopiedLength);
        CopiedLength += Node->String.Length;
    }
    
    Assert(CopiedLength == Length);
    
    string Result;
    Result.Str = StringMemory;
    Result.Length = CopiedLength;
    return Result;
}

