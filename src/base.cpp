
#include <stdarg.h> // va_list, va_copy, va_start, va_end
#include <stdio.h> // vsnprintf,
#include <string.h> // memset, memcpy, memmove

#include "platform.h"
#include "base.h"

///////////////////////////////////////////////////////////////////////
// Arena
//

// TODO: Make thread local
static arena *GlobalScratchArenaPool[2] = {};

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

bool MemoryIsEqual(u64 Size, void *A, void *B)
{
    Assert(A != nullptr);
    Assert(B != nullptr);
    return memcmp(A, B, Size) == 0;
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

u64 AlignDown(u64 Value, u64 Align)
{
    Assert(IsPowerOfTwo(Align));
    
    return Value & ~(Align - 1);
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
            Arena->Reserve = AlignedReserveSize;
            Arena->TempCount = 0;
        }
        else
        {
            PlatformMemoryFree(Memory, AlignedReserveSize);
        }
    }
    
    Assert(Arena);
    
    return Arena;
}

void FreeArena(arena *Arena)
{
    PlatformMemoryFree(Arena->Base, Arena->Reserve);
}

// If Size is 0, we return a correctly aligned pointer.
void *PushSize_(arena *Arena, u64 Size, u32 Alignment, u32 ArenaPushFlags)
{
    
    // It is not really necessary to properly align allocations on modern x64 processors (unless you are doing SIMD, or possibly on ARM), 
    // It may be undefined behaviour to access an unaligned type, but we are already ignoring strict aliasing.
    
    Assert(Alignment > 0);
    Assert(IsPowerOfTwo(Alignment));
    
    Assert(Arena->Pos >= ARENA_HEADER_SIZE);
    Assert(Arena->Pos <= Arena->Commit);
    Assert(Arena->Commit <= Arena->Reserve);
    Assert((Arena->Commit & (PAGE_SIZE - 1)) == 0);
    
    u8 *Result = nullptr;
    
    u64 AllocStart = AlignUp(Arena->Pos, Alignment);
    u64 AllocEnd   = AllocStart + Size;
    
    u64 NewPos = AllocEnd;
    
#if ARENA_GUARD_PAGES
    u64 GuardStart = AlignUp(AllocEnd, PAGE_SIZE); 
    u64 GuardEnd   = GuardStart + PAGE_SIZE; 
    NewPos = GuardEnd;
#endif
    
    if (NewPos < Arena->Reserve)
    {
        if (NewPos > Arena->Commit)
        {
            u64 TotalCommitSize = AlignUp(NewPos, COMMIT_CHUNK_SIZE);
            u64 NewCommit = ClampTop(TotalCommitSize, Arena->Reserve);
            u64 NewCommitSize = NewCommit - Arena->Commit;
            if (PlatformMemoryCommit(Arena->Base + Arena->Commit, NewCommitSize))
            {
                Arena->Commit = NewCommit;
            }
        }
        
        if (NewPos <= Arena->Commit)
        {
            Result = Arena->Base + AllocStart;
            Arena->Pos = NewPos;
            
#if ARENA_GUARD_PAGES
            PlatformMemoryGuard(Arena->Base + GuardStart, PAGE_SIZE);
            
            // The only cases where AllofStart + BytesToPageBoundary  is not aligned to the requested alignment
            // is when the passed size is not a multiple of the alignment. This should only occur where
            // PushSize_ is called directly, as structs and arrays of structs will have a size that is a multiple
            // of their alignment.
            u64 BytesToPageBoundary = AlignUp(AllocEnd, PAGE_SIZE) - AllocEnd;
            u64 AlignedOffset = AlignDown(BytesToPageBoundary, Alignment);
            
            u64 AllocStartGuardPageAdjacent = AlignDown(AllocStart + BytesToPageBoundary, Alignment);
            // AllocStart = AllocStartGuardPageAdjacent;
            // Checking if math is the same with new way to do it
            Assert(AllocStartGuardPageAdjacent == AllocStart + AlignedOffset);
            
            Result += AlignedOffset;
#endif
            
            if (ArenaPushFlags & ArenaPushFlags_ClearToZero)
            {
                MemoryZero(Size, Result);
            }
        }
    }
    
    Assert(Result);
    
    return Result;
}

void *PushCopy_(arena *Arena, u64 Size, void *Source, u32 Alignment)
{
    void *Memory = PushSize_(Arena, Size, Alignment, ArenaPushFlags_None);
    MemoryCopy(Size, Source, Memory);
    return Memory;
}

void PopToPosition(arena *Arena, u64 Position)
{
    Assert(Arena->Pos >= ARENA_HEADER_SIZE);
    Assert(Arena->Pos <= Arena->Commit);
    Assert(Arena->Commit <= Arena->Reserve);
    Assert((Arena->Commit & (PAGE_SIZE - 1)) == 0);
    
    // Prevent popping arena struct at start of allocated memory
    // or if the position is above the current arena pos.
    Assert(Position >= ARENA_HEADER_SIZE);
    
    u64 NewPos = Clamp(Position, (u64)ARENA_HEADER_SIZE, Arena->Pos);
    
    // Ensure that there is at most a commit chunk free
    u64 TestCommit = AlignUp(NewPos + COMMIT_CHUNK_SIZE, PAGE_SIZE);
    if (TestCommit < Arena->Commit)
    {
        u64 NewCommit = TestCommit;
        u64 DecommitSize = Arena->Commit - NewCommit;
        PlatformMemoryDecommit(Arena->Base + NewCommit, DecommitSize);
        Arena->Commit = NewCommit;
    }
    
    Assert(ClampTop(Arena->Pos, Arena->Commit) >= NewPos);
    
#if ARENA_GUARD_PAGES
    u64 RemoveGuardStart = AlignDown(NewPos, PAGE_SIZE); 
    u64 RemoveGuardEnd   = AlignUp(ClampTop(Arena->Pos, Arena->Commit), PAGE_SIZE); 
    u64 RemoveGuardSize  = RemoveGuardEnd - RemoveGuardStart;
    if (RemoveGuardSize > 0)
    {
        PlatformMemoryRemoveGuard(Arena->Base + RemoveGuardStart, RemoveGuardSize);
    }
#endif
    
    Arena->Pos = NewPos;
}

void PopSize(arena *Arena, u64 Size)
{
    Assert(Arena->Pos > Size);
    PopToPosition(Arena, Arena->Pos - Size);
}

void ClearArena(arena *Arena)
{
    PopToPosition(Arena, ARENA_HEADER_SIZE);
}

temp_arena BeginTempArena(arena *Arena)
{
    temp_arena Temp = {};
    Temp.Arena = Arena;
    Temp.StartPos = Arena->Pos; 
    Temp.TempIndex = Arena->TempCount;
    
    Arena->TempCount += 1;
    
    return Temp;
}

void EndTempArena(temp_arena TempArena)
{
    arena *Arena = TempArena.Arena;
    // NOTE: This forces GetScratch() ReleaseScratch() pairs to be matched
    // You can't call ReleaseScratch() on a Arena lower on the stack to avoid releasing
    // on higher on the stack.
    // NOTE: Lets wait and see if this is useful or not.
    Assert(Arena->TempCount-1 == TempArena.TempIndex);
    
    PopToPosition(Arena, TempArena.StartPos);
    Arena->TempCount -= 1;
}

void CheckArena(arena *Arena)
{
    Assert(Arena->TempCount == 0);
}

temp_arena GetScratchImpl(arena **Conflicts, u32 ConflictCount)
//temp_arena GetScratchImpl(std::array<arena *> &Array, u32 Count)
{
    if (GlobalScratchArenaPool[0] == 0)
    {
        static_assert(ArrayCount(GlobalScratchArenaPool) == 2, "Must be only two scratch arenas");
        GlobalScratchArenaPool[0] = CreateArena(GB(16));
        GlobalScratchArenaPool[1] = CreateArena(GB(16));
    }
    
    arena *Arena = GlobalScratchArenaPool[0];
    if (ConflictCount > 0) 
    {
        for (u32 i = 0; i < ConflictCount; ++i) 
        {
            if (GlobalScratchArenaPool[0] == Conflicts[i]) 
            {
                Arena = 0;
                break;
            }
        }
        
        if (!Arena) 
        {
            Arena = GlobalScratchArenaPool[1];
            for (u32 i = 0; i < ConflictCount; ++i) 
            {
                if (GlobalScratchArenaPool[1] == Conflicts[i]) 
                {
                    Arena = 0;
                    break;
                }
            }
        }
    }
    
    Assert(Arena && "Unable to get non-conflicting arena");
    
    return BeginTempArena(Arena);
}

temp_arena GetScratch()
{
    return GetScratchImpl(0, 0);
}

temp_arena GetScratch(arena *Arena)
{
    return GetScratchImpl(&Arena, 1);
}

temp_arena GetScratch(arena *Arena1, arena *Arena2)
{
    arena *ArenaArray[2] = {Arena1, Arena2};
    return GetScratchImpl(ArenaArray, 2);
}

void ReleaseScratch(temp_arena ScratchMemory)
{
    Assert(ScratchMemory.Arena == GlobalScratchArenaPool[0] || 
           ScratchMemory.Arena == GlobalScratchArenaPool[1]);
    EndTempArena(ScratchMemory);
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

string CreateString(u8 *CString)
{
    string Result;
    Result.Str = CString;
    Result.Length = StringLength(CString);
    return Result;
}

string CreateString(u8 *StringData, u64 Length)
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
    u8 *Memory = (u8 *)PushCopy_(Arena, Length, String, 1);
    return Memory;
}

u8 *PushCString(arena *Arena, string String)
{
    u8 *StringZ  = PushArrayNoZero(Arena, String.Length + 1, u8);
    MemoryCopy(String.Length, String.Str, StringZ);
    StringZ[String.Length] = 0;
    return StringZ;
}

string PushString(arena *Arena, string String)
{
    u8 *StringData = (u8 *)PushCopy_(Arena, String.Length, String.Str, 1);
    return CreateString(StringData, String.Length);
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

string PushStringfArgs(arena *Arena, char *Format, va_list Args)
{
    u64 Length = 0;
    u64 Capacity = 1024;
    u8 *Buffer = PushArrayNoZero(Arena, Capacity, u8);
    
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
    else if ((u64)RequiredCharCount >= Capacity)
    {
        // Truncated
        // Returns the number of chars that would have been written (not including the null terminator).
        // If RequiredCharCount == StringLength (not including null terminator) then the last char was not 
        // written as a null terminator must be written instead.
        
        PopSize(Arena, Capacity);
        u64 NewCapacity = (u64)RequiredCharCount + 1;
        Buffer = PushArrayNoZero(Arena, NewCapacity, u8);
        
        int WrittenCharCount = vsnprintf((char *)Buffer, NewCapacity, Format, ArgsCopy);
        if (WrittenCharCount < 0)
        {
            Length = 0;
            Assert(0);
        }
        else
        {
            // Leave null terminated in allocated string
            Length = (u64)WrittenCharCount;
        }
    }
    else
    {
        // Success
        Length = (u64)RequiredCharCount;
    }
    
    va_end (ArgsCopy);
    
    return CreateString(Buffer, Length);
}


string PushStringf(arena *Arena, char *Format, ...)
{
    va_list Args;
    va_start (Args, Format);
    string Result = PushStringfArgs(Arena, Format, Args);
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

bool IsNumber(u8 c)
{
    return (('0' <= c) && (c <= '9'));
}

bool IsAlphaNumeric(u8 c)
{
    return IsAlpha(c) || IsNumber(c);
}

bool IsWhitespace(u8 c)
{
    return (c == ' ' || c == '\r' || c == '\n' || c == '\t' || c == '\f' || c == '\v');
}

bool IsSlash(u8 c)
{
    return (c == '\\' || c == '/');
}

u8 ToUpper(u8 c)
{
    return IsLower(c) ? c - 32u : c;
}

u8 ToLower(u8 c)
{
    return IsUpper(c) ? c + 32u : c;
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

string StringPrefix(string String, u64 N)
{
    String.Length = ClampTop(N, String.Length);
    return String;
}

string StringSuffix(string String, u64 N)
{
    u64 SuffixLength = Minimum(N, String.Length);
    String.Str += (String.Length - SuffixLength);
    String.Length = SuffixLength;
    return String;
}

string SubstrRange(string String, u64 First, u64 OnePastLast)
{
    Assert(OnePastLast >= First);
    //   v   v
    // abcdef
    First = ClampTop(First, String.Length);
    OnePastLast = ClampTop(OnePastLast, String.Length);
    
    String.Str += First;
    String.Length = OnePastLast - First;
    return String;
}

string Substr(string String, u64 First, u64 Length)
{
    return SubstrRange(String, First, First + Length);
}

string StringSkip(string String, u64 N)
{
    N = ClampTop(N, String.Length);
    String.Str = String.Str + N;
    String.Length -= N;
    return String;
}

string StringChop(string String, u64 N)
{
    N = ClampTop(N, String.Length);
    String.Length -= N;
    return String;
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

u64 StringFindLastChar(string String, u8 Char)
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


u64 StringFindStr(string Haystack, string Needle)
{
    u64 Position = Haystack.Length;
    
    if (Needle.Length > 0 && Haystack.Length >= Needle.Length) 
    {
        for (u64 i = 0; i < Haystack.Length - Needle.Length + 1; ++i)
        {
            if (Needle.Str[0] == Haystack.Str[i])
            {
                bool Found = true;
                for (u64 j = 1; j < Needle.Length; ++j)
                {
                    if (Needle.Str[j] != Haystack.Str[i + j])
                    {
                        Found = false;
                        break;
                    }
                }
                
                if (Found)
                {
                    Position = i;
                    break;
                }
            }
        }
    }
    
    return Position;
}

bool StringContainsChar(string String, u8 Char)
{
    return StringFindChar(String, Char) < String.Length;
}

bool StringContainsStr(string String, string Substr)
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

u64 StringFindLastSlash(string Path)
{
    u64 Position = Path.Length;
    for (u64 i = Path.Length - 1; i != (u64)-1; --i)
    {
        if (IsSlash(Path.Str[i]))
        {
            Position = i;
            break;
        }
    }
    
    return Position;
}

void RemoveExtension(string *File)
{
    // Files can have multiple dots, and only last is the real extension.
    
    // If no dot is found Length stays the same
    u64 DotPosition = StringFindLastChar(*File, '.');
    File->Length = DotPosition;
}

// If Path has no slashes (it is not an absolute path) then a zero length string is returned
string FilenameFromPath(string Path)
{
    u64 LastSlash = StringFindLastSlash(Path);
    Path = StringSkip(Path, LastSlash + 1);
    return Path;
}

// If Path has no slashes then then a zero length string is returned
// Includes slash after directory
string DirectoryFromPath(string Path)
{
    string Directory = {};
    u64 LastSlash = StringFindLastSlash(Path);
    if (LastSlash < Path.Length)
    {
        Directory = StringPrefix(Path, LastSlash + 1);
    }
    
    return Directory;
}

void StringListAppend(arena *Arena, string_list *List, string String)
{
    string_node *Node = PushStruct(Arena, string_node);
    Node->String = PushString(Arena, String);
    Node->Next = nullptr;
    
    if (List->Head == nullptr)
    {
        List->Head = Node;
    }
    else
    {
        List->Tail->Next = Node;
    }
    
    List->Tail = Node;
    List->Length += String.Length;
}

void StringListAppend(arena *Arena, string_list *List, char *String)
{
    StringListAppend(Arena, List, CreateString((u8 *)String));
}

string StringListJoin(arena *Arena, string_list *List)
{
    u8 *StringMemory = PushArrayNoZero(Arena, List->Length, u8);
    
    u64 CopiedLength = 0;
    for (string_node *Node = List->Head; Node; Node = Node->Next)
    {
        MemoryCopy(Node->String.Length, Node->String.Str, StringMemory + CopiedLength);
        CopiedLength += Node->String.Length;
    }
    
    string Result = {};
    Result.Str = StringMemory;
    Result.Length = CopiedLength;
    return Result;
}


string_list StringListSplit(arena *Arena, string String, string Splits)
{
    string_list List = {};
    u64 StartIndex = 0;
    u64 At = 0;
    for (; At < String.Length; ++At)
    {
        bool MatchedSplit = false;
        for (u64 i = 0; i < Splits.Length; ++i)
        {
            if (String.Str[At] == Splits[i]) 
            {
                MatchedSplit = true;
                break;
            }
        }
        
        if (MatchedSplit) 
        {
            if (StartIndex < At)
            {
                StringListAppend(Arena, &List, SubstrRange(String, StartIndex, At));
            }
            StartIndex = At + 1;
        }
    }
    
    if (StartIndex < At) 
    {
        StringListAppend(Arena, &List, SubstrRange(String, StartIndex, String.Length));
    }
    
    return List;
}
