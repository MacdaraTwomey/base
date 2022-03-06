
#include <stdarg.h> // va_list, va_copy, va_start, va_end
#include <stdio.h> // vsnprintf,
#include <string.h> // memset, memcpy, memmove
#include <math.h>

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
//#define COMMIT_CHUNK_SIZE MB(32)
#define COMMIT_CHUNK_SIZE (PAGE_SIZE * 8) // TODO

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

void *PushSize_(arena *Arena, u64 Size, u32 Alignment, u32 ArenaPushFlags)
{
    // It is not really necessary to properly align allocations on modern x64 processors (unless you are doing SIMD, or possibly on ARM), 
    // It may be undefined behaviour to access an unaligned type, but we are already ignoring strict aliasing.
    
    Assert(Size > 0);
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
#if ARENA_GUARD_PAGES
            // The only cases where Offset is not aligned to the requested alignment is when 
            // the passed size is not a multiple of the alignment. This should only occur where
            // PushSize_ is called directly, as structs and arrays of structs will have a size that is a multiple
            // of their alignment.
            u64 BytesToPageBoundary = AlignUp(AllocEnd, PAGE_SIZE) - AllocEnd;
            u64 AllocStartGuardPageAdjacent = AlignDown(AllocStart + BytesToPageBoundary, Alignment);
            PlatformMemoryGuard(Arena->Base + GuardStart, PAGE_SIZE);
            
            AllocStart = AllocStartGuardPageAdjacent;
#endif
            
            Result = Arena->Base + AllocStart;
            Arena->Pos = NewPos;
            
            if (ArenaPushFlags & ArenaPushFlags_ClearToZero)
            {
                MemoryZero(Size, Result);
            }
            
        }
    }
    
    Assert(Result);
    
    return Result;
}


#if 0
void *PushSize_(arena *Arena, u64 Size, u32 Alignment, u32 ArenaPushFlags)
{
    // It is not really necessary to properly align allocations on modern x64 processors (unless you are doing SIMD, or possibly on ARM), 
    // It may be undefined behaviour to access an unaligned type, but we are already ignoring strict aliasing.
    
    Assert(Size > 0);
    Assert(Alignment > 0);
    Assert(IsPowerOfTwo(Alignment));
    
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
#endif

void *PushCopy_(arena *Arena, u64 Size, void *Source, u32 Alignment)
{
    void *Memory = PushSize_(Arena, Size, Alignment, NoClear());
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

string PushFormatStringArgs(arena *Arena, char *Format, va_list Args)
{
    u64 Length = 0;
    u64 Capacity = 1024;
    u8 *Buffer = PushArray(Arena, Capacity, u8, NoClear());
    
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
        Buffer = PushArray(Arena, NewCapacity, u8);
        
        int WrittenCharCount = vsnprintf((char *)Buffer, NewCapacity, Format, ArgsCopy);
        if (WrittenCharCount < 0)
        {
            Length = 0;
            Assert(0);
        }
        else
        {
            PopSize(Arena, NewCapacity - WrittenCharCount); // TODO: Is it necessary to pop one byte?
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

u64 StringFindLastSlash(string Path)
{
    u64 Position = Path.Length;
    for (u64 i = Path.Length - 1; i != static_cast<u64>(-1); --i)
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
    StringSkip(&Path, LastSlash + 1);
    return Path;
}

// If Path has no slashes then then a zero length string is returned
// Includes slash after directory
string DirectoryFromPath(string Path)
{
    // /abc/dev/file.exe
    string Directory = {};
    u64 LastSlash = StringFindLastSlash(Path);
    if (LastSlash < Path.Length)
    {
        Directory = StringGetPrefix(Path, LastSlash + 1);
    }
    
    return Directory;
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
    string_node *Node = PushStruct(Arena, string_node);
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
    Append(Arena, CreateString((u8 *)String));
}

string string_builder::GetString(arena *Arena)
{
    u8 *StringMemory = PushArray(Arena, Length, u8);
    
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


///////////////////////////////////////////////////////////////////////
// Math
//

//
// Scalar operations
//

inline f32 Cos(f32 Radians)
{
    return cosf(Radians);
}

inline f32 Sin(f32 Radians)
{
    return sinf(Radians);
}

inline f32 Atan2(f32 Y, f32 X)
{
    return atan2f(Y, X);
}

inline f32 Square(f32 A)
{
    return A * A;
}

inline f32 SquareRoot(f32 A)
{
    return sqrtf(A);
}

inline f32 Pow(f32 Base, f32 Exponent)
{
    return powf(Base, Exponent);
}

inline f32 AbsoluteValue(f32 A)
{
    return fabsf(A);
}

inline f32 Round(f32 A)
{
    return roundf(A);
}

inline f32 Floor(f32 A)
{
    return floorf(A);
}

inline f32 Lerp(f32 A, f32 t, f32 B)
{
    return (1.0f - t)*A + t*B;
}

inline f32 Clamp01(f32 Value)
{
    return Clamp(0.0f, Value, 1.0f);
}

constexpr f32 RadFromDeg(f32 Degrees)
{
    return Degrees * 0.01745329251994329576923690768489f;
}

constexpr f32 DegFromRad(f32 Radians)
{
    return Radians * 57.295779513082320876798154814105f;
}

//
// Vector operations
//

inline v2 V2(f32 x, f32 y)
{
    return v2{x, y};
}

inline v3 V3(f32 x, f32 y, f32 z)
{
    return v3{x, y, z};
}

inline v3 V3(v2 xy, f32 z)
{
    return v3{xy.x, xy.y, z};
}

inline v4 V4(f32 x, f32 y, f32 z, f32 w)
{
    return v4{x, y, z, w};
}

inline v4 V4(v3 xyz, f32 w)
{
    return v4{xyz.x, xyz.y, xyz.z, w};
}

inline v2 operator+(v2 A, v2 B)          { return v2{A.x+B.x, A.y+B.y}; } 
inline v2 operator-(v2 A, v2 B)          { return v2{A.x-B.x, A.y-B.y}; } 
inline v2 operator-(v2 A)                { return v2{-A.x,   -A.y}; }           

inline v2 operator*(f32 Scalar, v2 A)    { return v2{Scalar*A.x, Scalar*A.y}; }     
inline v2 operator*(v2 A, f32 Scalar)    { return Scalar * A; }

inline v2& operator+=(v2& A, v2 B)       { return (A = A + B); }
inline v2& operator-=(v2& A, v2 B)       { return (A = A - B); }
inline v2& operator*=(v2& A, f32 Scalar) { return (A = Scalar * A); }

inline v3 operator+(v3 A, v3 B)          { return v3{A.x+B.x, A.y+B.y, A.z+B.z}; }
inline v3 operator-(v3 A, v3 B)          { return v3{A.x-B.x, A.y-B.y, A.z-B.z}; }
inline v3 operator-(v3 A)                { return v3{-A.x,   -A.y,    -A.z}; }

inline v3 operator*(f32 Scalar, v3 A)    { return v3{Scalar*A.x, Scalar*A.y, Scalar*A.z}; }
inline v3 operator/(f32 Scalar, v3 A)    { return v3{Scalar/A.x, Scalar/A.y, Scalar/A.z};}
inline v3 operator*(v3 A, f32 Scalar)    { return Scalar * A; }
inline v3 operator/(v3 A, f32 Scalar)    { return (1.0f / Scalar) * A; } // Faster less accurate version

inline v3& operator+=(v3& A, v3 B)       { return (A = A + B); }
inline v3& operator-=(v3& A, v3 B)       { return (A = A - B); }
inline v3& operator*=(v3& A, f32 Scalar) { return (A = Scalar * A); }
inline v3& operator/=(v3& A, f32 Scalar) { return (A = A / Scalar); }

inline v4 operator+(v4 A, v4 B)          { return v4{A.x+B.x, A.y+B.y, A.z+B.z, A.w+B.w}; }
inline v4 operator-(v4 A, v4 B)          { return v4{A.x-B.x, A.y-B.y, A.z-B.z, A.w-B.w}; }
inline v4 operator-(v4 A)                { return v4{-A.x,   -A.y,    -A.z,    -A.w}; }

inline v4 operator*(f32 Scalar, v4 A)    { return v4{Scalar*A.x, Scalar*A.y, Scalar*A.z, Scalar*A.w}; }
inline v4 operator/(f32 Scalar, v4 A)    { return v4{Scalar/A.x, Scalar/A.y, Scalar/A.z, Scalar/A.w }; } 
inline v4 operator*(v4 A, f32 Scalar)    { return Scalar*A; }
inline v4 operator/(v4 A, f32 Scalar)    { return (1.0f / Scalar) * A; }

inline v4& operator+=(v4& A, v4 B)       { return (A = A + B); }
inline v4& operator-=(v4& A, v4 B)       { return (A = A - B); }
inline v4& operator*=(v4& A, f32 Scalar) { return (A = Scalar * A); }
inline v4& operator/=(v4& A, f32 Scalar) { return (A = A / Scalar); }

inline f32 Inner(v2 A, v2 B)
{
    return A.x*B.x + A.y*B.y;
}

inline v2 Perp(v2 A)
{
    return v2{-A.y, A.x};
}

inline v2 Hadamard(v2 A, v2 B)
{
    return v2{A.x*B.x, A.y*B.y};
}

inline f32 LengthSq(v2 A)
{
    return Inner(A, A);
}

inline f32 Length(v2 A)
{
    return SquareRoot(LengthSq(A));
}

inline v2 Normalize(v2 A)
{
    return A * (1.0f / Length(A));
}

inline v2 Clamp01(v2 Value)
{
    return v2{Clamp01(Value.x), Clamp01(Value.y)};
}


inline v3 Lerp(v3 A, f32 t, v3 B)
{
    return (1.0f - t)*A + t*B;
}

inline v3 Hadamard(v3 A, v3 B)
{
    return v3{A.x*B.x, A.y*B.y, A.z*B.z};
}

inline f32 Inner(v3 A, v3 B)
{
    return A.x*B.x + A.y*B.y + A.z*B.z;
}

inline v3 Cross(v3 A, v3 B)
{
    return v3{
        A.y*B.z - A.z*B.y,
        A.z*B.x - A.x*B.z,
        A.x*B.y - A.y*B.x,
    };
}

inline f32 LengthSq(v3 A)
{
    return Inner(A, A);
}

inline f32 Length(v3 A)
{
    return SquareRoot(LengthSq(A));
}

inline v3 Normalize(v3 A)
{
    return A * (1.0f / Length(A));
}

inline v3 NOZ(v3 A)
{
    v3 Result = {};
    
    f32 LengthSquared = LengthSq(A);
    if(LengthSquared > Square(0.0001f))
    {
        Result = A * (1.0f / SquareRoot(LengthSquared));
    }
    
    return Result;
}

inline v3 Clamp01(v3 Value)
{
    return v3{Clamp01(Value.x), Clamp01(Value.y), Clamp01(Value.z)};
}


inline v4 Hadamard(v4 A, v4 B)
{
    return v4{A.x*B.x, A.y*B.y, A.z*B.z, A.w*B.w};
}

inline f32 Inner(v4 A, v4 B)
{
    return A.x*B.x + A.y*B.y + A.z*B.z + A.w*B.w;
}

inline f32 LengthSq(v4 A)
{
    return Inner(A, A);
}

inline f32 Length(v4 A)
{
    return SquareRoot(LengthSq(A));
}

inline v4 Clamp01(v4 Value)
{
    return v4{Clamp01(Value.x), Clamp01(Value.y), Clamp01(Value.z), Clamp01(Value.w)};
}

inline v4 Lerp(v4 A, f32 t, v4 B)
{
    return (1.0f - t)*A + t*B;
}

//
// Matrix operations
//

v4 operator*(m4x4 M, v4 V)
{
    v4 Result {
        M.E[0][0] * V.x + M.E[0][1] * V.y + M.E[0][2] * V.z + M.E[0][3] * V.w,
        M.E[1][0] * V.x + M.E[1][1] * V.y + M.E[1][2] * V.z + M.E[1][3] * V.w,
        M.E[2][0] * V.x + M.E[2][1] * V.y + M.E[2][2] * V.z + M.E[2][3] * V.w,
        M.E[3][0] * V.x + M.E[3][1] * V.y + M.E[3][2] * V.z + M.E[3][3] * V.w,
    };
    
    return Result;
}

m4x4 operator*(m4x4 A, m4x4 B)
{
    m4x4 Result  = {};
    for (s32 Row = 0; Row < 4; ++Row)
    {
        for (s32 Col = 0; Col < 4; ++Col)
        {
            Result.E[Row][Col] = 
                A.E[Row][0] * B.E[0][Col] +
                A.E[Row][1] * B.E[1][Col] +
                A.E[Row][2] * B.E[2][Col] +
                A.E[Row][3] * B.E[3][Col];
        }
    }
    
    return Result;
}

m4x4 Identity()
{
    return m4x4 {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
};

m4x4 RotateZ(f32 Angle)
{
    f32 c = Cos(Angle);
    f32 s = Sin(Angle);
    return m4x4 {
        c,    -s,   0.0f, 0.0f,
        s,    c,    0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

m4x4 RotateX(f32 Angle)
{
    f32 c = Cos(Angle);
    f32 s = Sin(Angle);
    return m4x4 {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, c,    -s,   0.0f,
        0.0f, s,    c,    0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

m4x4 RotateY(f32 Angle)
{
    f32 c = Cos(Angle);
    f32 s = Sin(Angle);
    return m4x4 {
        c,    0.0f, s,    0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        -s,   0.0f, c,    0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

m4x4 Scale(f32 S)
{
    return m4x4 {
        S, 0.0f, 0.0f, 0.0f,
        0.0f, S, 0.0f, 0.0f,
        0.0f, 0.0f, S, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

m4x4 Translation(v3 T)
{
    return m4x4 {
        1.0f, 0.0f, 0.0f, T.x,
        0.0f, 1.0f, 0.0f, T.y,
        0.0f, 0.0f, 1.0f, T.z,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

m4x4 LookAt(v3 CameraP, v3 TargetP, v3 WorldUp)
{
    /*
          CameraP = (1, 0, 0) in our z-is-up worldspace
          CameraTarget = (1, 0, 0) in our z-is-up worldspace
    
LookAt performs two transformations, translating the scene from the eye (camera) position to the origin,
then rotating the scene in the reverse orientation of the camera.

View = Rotate * Tranlate 
   
Translate = 
|1 0 0 -CameraP.x|
|0 1 0 -CameraP.y|
|0 0 1 -CameraP.z|
|0 0 0 1         |

Rotate:

GlobalUp = (0, 1, 0) TODO: Maybe this is (0, 0, 1) in our worldspace
front = Normalise(CameraP - CameraTarget)
 right = Cross(GlobalUp, front) // normalised by cross
up = Cross(front, right) 

Rotate is the inverse of the matrix made with these these basis vectors.
As the matrix has orthogonal unit length vectors, its inverse is equal to the transpose.

Rotate = 
    |rx ux fx 0|^-1     |rx ry rz 0|
    |ry uy fy 0|     =  |ux uy uz 0|
    |rz uz fz 0|        |fx fy yz 0|
    |0  0  0  1|        |0  0  0  1|

    View = 
|rx ry rz 0| |1 0 0 -Px|   |ry ry rz -rxPx-ryPy-rzPz|
|ux uy uz 0| |0 1 0 -Py| = |ux uy uz -uxPx-uyPy-uzPz|
|fx fy yz 0| |0 0 1 -Pz|   |fx fy fz -fxPx-fyPy-fzPz|
 |0  0  0  1| |0 0 0 1  |   |0  0  0         1       |

* Where P is the camera (eye) position

        */
    
    // We set up a set of basis vectors matching what OpenGL expects
    // So all our world coordinates will be in OpenGL coordinate system (y is up, looking in -z direction)
    v3 Front = Normalize(CameraP - TargetP);    // Pointing away from scene (into camera as OpenGL expects) 
    v3 Side = Normalize(Cross(WorldUp, Front));  // TODO: If Front and Up are aligned this is bad
    v3 Up = Normalize(Cross(Front, Side));
    
#if 0
    // The two matrices that make CameraViewInverse
    m4x4 RotateInv = {
        Side.x,  Side.y,  Side.z,  0.0f,
        Up.x,    Up.y,    Up.z,    0.0f,
        Front.x, Front.y, Front.z, 0.0f,
        0.0f,    0.0f,    0.0f,    1.0f 
    };
    
    m4x4 Translate = {
        1.0f, 0,    0,     -CameraP.x,
        0,    1.0f, 0,     -CameraP.y,
        0,    0,    1.0f,  -CameraP.z,
        0,    0,    0,     1.0f
    };
    
    m4x4 CameraViewInverse = RotateInv * Translate;
#endif
    
    m4x4 CameraViewInverse = {
        Side.x,  Side.y,  Side.z,  -Inner(Side,  CameraP),
        Up.x,    Up.y,    Up.z,    -Inner(Up,    CameraP),
        Front.x, Front.y, Front.z, -Inner(Front, CameraP),
        0.0f,    0.0f,    0.0f,    1.0f
    };
    
    return CameraViewInverse;
    
    
    /* NOTE:
This can also be thought of as multiplying a point with the camera transform matrix.
|a b c| (camera x axis)  |x|    |x dot camera x axis|
|d e f| (camera y axis)  |y|  = |y dot camera y axis|
 |g h i| (camera z axis)  |z|    |z dot camera z axis|

Where we measure how far along the camera's axes the point is, thus transforming the world coordinate
into view space.
"The row perspective of a matrix".
*/
}

m4x4 PerspectiveProjection(f32 AspectWidthOverHeight, f32 FocalLength, f32 NearClipPlane, f32 FarClipPlane)
{
    // L is focal length
    // A is Aspect ration (Width / Height)
    
    // L is also equal to cotangent(theta) (or equivalently 1/tan(theta))
    // A theta angle of FOV/2 is commonly used instead of focal length for constructing the projection matrix, 
    // (FOV is the vertical viewing angle, divided by two because it is the angle from the centre to the top 
    // of the focal plane). The horizontal viewing angle can also be used (with a different formula).
    // As L increased theta increases and cot(theta) increases. This is the focal plane moving closer into
    // the frustum and scaling the image with a zoom effect. 
    // The Near and Far planes are defined in world coordinates.
    
    // OpenGL matrix you see online will be:   
    // L/A  0   0              0
    // 0    L   0              0
    // 0    0   -(F+N)/(F-N)  -2NF/(F-N)
    // 0    0   -1             0
    // This matrix has a negated third row, which serves to flip the projected image on the z axis.
    // Although OpenGL has the -z axis point away from the camera, in the depth buffer smaller values
    // are considered closer, hence the flip.
    
    // Maps a frustum volume into a cube surrounding the camera at (0, 0, 0) with corner coordinates of -1 to 1.
    // The resulting coordinates are NDC being from -1 to 1.
    
    f32 a = AspectWidthOverHeight;
    f32 l = FocalLength;
    
    f32 n = NearClipPlane; 
    f32 f = FarClipPlane; 
    
    f32 d = -(f+n) / (f-n);
    f32 e = -(2*f*n) / (f-n);
    
    m4x4 Result = {
        l/a, 0,  0,  0,
        0,   l,  0,  0,
        0,   0,  d,  e,
        0,   0, -1,  0,
    };
    
    return Result;
}

m4x4 OrthographicProjection3D(f32 Left, f32 Right, f32 Bottom, f32 Top, f32 Near, f32 Far)
{
    // Map the left and right screen coordinates to -1 to 1 (For OpenGL)
    // l <= x <= r            If x is a point within left and right:
    // 0 <= x-l <= r-l    Make the left side 0 by subtracting l from all terms
    // 0 <= x-l / (r-l) <= 1   Make the right side 1 by dividing by (r - l)
    // 0 <= 2((x-l) / (r-l)) <= 2 
    // -1 <= 2((x-l) / (r-l)) - 1 <= 1 Multiply by 2 and subtract to to map to [-1, 1]
    
    // -1 <= 2((x-l) / (r-l)) - ((r-l)/(r-l)) <= 1   Replace -1 with ((r-l)/(r-l))
    // -1 <= (2x-l-r) / (r-l) <= 1   Simplify
    // -1 <= 2x / (r-l) - (r+l) / (r-l) <= 1   Gives us the formula to transform x
    // x' = (2 / (r-l)) * x - (r+l) / (r-l)
    
    // Then by using this in the rows of our matrix 
    // vec = (x, 0, 0, 1) dot matrix x row = (2/(r-l), 0, 0, -(r+l)/(r-l))  
    //     = x'
    // Giving us the x component for the projected vector.
    // This works similarly for y and z components (z is negated as the camera faces the -z axis)
    
    // Maps a rectangular volume to a cube centred at (0, 0, 0) with with corners having coordinates -1 to 1
    // The resulting coordinates are NDC which are from -1 to 1 for each component.
    
    f32 a = 2.0f / (Right - Left);
    f32 b = 2.0f / (Top - Bottom);
    f32 c = -2.0f / (Far - Near); 
    
    f32 d = -(Right + Left) / (Right - Left);
    f32 e = -(Top + Bottom) / (Top - Bottom);
    f32 f = -(Far + Near) / (Far - Near);
    
    m4x4 Result = {
        a,  0,  0,  d,
        0,  b,  0,  e,
        0,  0,  c,  f,
        0,  0,  0,  1,
    };
    
    return Result;
}

m4x4 OrthographicProjection2D(f32 Left, f32 Right, f32 Bottom, f32 Top)
{
    f32 a = 2.0f / (Right - Left);
    f32 b = 2.0f / (Top - Bottom);
    
    f32 d = -(Right + Left) / (Right - Left);
    f32 e = -(Top + Bottom) / (Top - Bottom);
    
    m4x4 Result = {
        a,  0,  0,  d,
        0,  b,  0,  e,
        0,  0,  -1, 0,
        0,  0,  0,  1, 
    };
    
    return Result;
}

#if 0
v3 Orbit(v3 CameraP, v3 Target, f32 AzimuthRadians, f32 InclinationRadians)
{
    // Transform to spherical coordinates, add azimuth and inclination, then transform back.
    // Assumes z-is-up right handed coordinate system.
    
    v3 P = CameraP - Target;
    
    f32 r = Length(P);
    P = NOZ(P);
    
    f32 Inclination = acos(P.z) + InclinationRadians;
    f32 Azimuth = atan2(P.y, P.x) + AzimuthRadians;
    
    Inclination = Clamp(0.01f, Inclination, Pi32 - 0.01f);
    
    f32 x = cos(Azimuth) * sin(Inclination);
    f32 y = sin(Azimuth) * sin(Inclination);
    f32 z = cos(Inclination);
    v3 RotatedP = r * V3(x, y, z);
    
    v3 Result = RotatedP + Target;
    
    return Result;
}
#endif
