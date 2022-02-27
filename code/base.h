#pragma once

#include <stdint.h>

///////////////////////////////////////////////////////////////////////
// Platform detection
//

#if defined(_WIN32)
#define BASE_OS_WINDOWS 1
#elif defined(__linux__)
#define BASE_OS_LINUX 1
#elif defined(__APPLE__) || defined(__MACH__)
#define BASE_OS_MAC 1
#elif defined(__unix__)
#define BASE_OS_UNIX 1               // Some non-linux non-apple unix variety
#else
#error "Could not detect operating system"
#endif

#if defined(_MSC_VER)
#define BASE_COMPILER_MSVC 1
#elif defined(__GNUC__)
#define BASE_COMPILER_GCC 1
#elif defined(__clang__)
#define BASE_COMPILER_CLANG 1
#else
#error "Could not detect compiler"
#endif

#if defined(_M_IX86) || defined(__i386__) 
#define BASE_ARCH_X86 1              // 32-bit x86 architecture
#elif defined(_M_X64) || defined(__x86_64__) 
#define BASE_ARCH_X64 1              // 64-bit x86 architecture
#elif defined(__arm__)
#define BASE_ARCH_ARM 1              // Could be 32 or 64-bit (could use __aarch64__ to detect further)
#else
#error "Could not detect CPU architecture"
#endif

#if !BASE_OS_WINDOWS
#error "This operating system is currently not supported"
#endif

///////////////////////////////////////////////////////////////////////
// Assert
//

#if !defined(BASE_DEBUG_TRAP)

#if defined(BASE_OS_WINDOWS)
// On windows use __debugbreak
extern void __cdecl __debugbreak(void); // To avoid including intrin.h
#define BASE_DEBUG_TRAP() __debugbreak() 
#else

#if defined(BASE_OS_MAC)
#error "Not implemented"
#elif defined(BASE_ARCH_ARM)
#error "Not implemented"
#elif defined(BASE_ARCH_X64) || defined(BASE_ARCH_X86)
#define BASE_DEBUG_TRAP() __builtin_trap()
#else
#define BASE_DEBUG_TRAP() (*(int *)0 = 0;)
#endif

#endif // defined(BASE_OS_WINDOWS)
#endif // !defined(BASE_DEBUG_TRAP)


#define BASE_STRINGIFY_(Str) #Str
#define BASE_STRINGIFY(Str) BASE_STRINGIFY_(Str)

#define CONCAT_(x, y) x##y // Can concat literal characters xy if x and y are macro definitions
#define CONCAT(x, y) CONCAT_(x, y) // So macro expand x and y first

// If BASE_DEBUG is not defined then we enable it, 
// If BASE_DEBUG is defined but with no value then we still get assertions, 
// if BASE_DEBUG is zero then we don't have assertions (unless they are specifically turned on), 
// if BASE_DEBUG is one then we have assertions (unless they are specifically turned off).
#if !defined(BASE_DEBUG)
#define BASE_DEBUG 1
#endif

void BaseAssertPrint_(const char *Format, ...);

#if (defined(BASE_ENABLE_ASSERT) && (BASE_ENABLE_ASSERT == 0)) || (!defined(BASE_ENABLE_ASSERT) && BASE_DEBUG == 0)
#define Assert(Cond) ((void)0)
#else
// In this instance we don't want to macro expand Cond, so that we can print our macros without substitution.
#define Assert(Cond) do { \
if (!static_cast<bool>((Cond))) { \
BaseAssertPrint_("%s:%s():%i: Assertion: `%s' failed.\n", __FILE__, __func__, __LINE__, #Cond); \
BASE_DEBUG_TRAP(); \
} \
} while(0) 

#endif


///////////////////////////////////////////////////////////////////////
// Helpers
//

template<class T>
constexpr const T& Clamp(const T& Value, const T& Low, const T& High)
{
    Assert(Low <= High);
    return (Value < Low) ? Low : (High < Value) ? High : Value;
}

// TODO: Do warnings catch signed unsigned comparison?
#define Minimum(a, b) ((a) < (b) ? (a) : (b))
#define Maximum(a, b) ((a) > (b) ? (a) : (b))
//#define Clamp(Value, Low, High) (((Value) < (Low)) ? (Low) : ((High) < (Value)) ? (High) : (Value))
#define ClampTop(a, b)    Minimum((a), (b))
#define ClampBottom(a, b) Maximum((a), (b))

#define KB(n) ((n) * 1024LLU)
#define MB(n) KB((n) * 1024LLU)
#define GB(n) MB((n) * 1024LLU)
#define TB(n) GB((n) * 1024LLU)

template <typename T, size_t n>
constexpr size_t ArrayCount(T(&)[n])
{
    return n;
}

///////////////////////////////////////////////////////////////////////
// Types
//

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int32_t  b32;
typedef int16_t  b16;
typedef int8_t   b8;

typedef float    f32;
typedef double   f64;

static_assert(sizeof(u8)  == 1, "");
static_assert(sizeof(u16) == 2, "");
static_assert(sizeof(u32) == 4, "");
static_assert(sizeof(u64) == 8, "");

static_assert(sizeof(f32) == 4, "");
static_assert(sizeof(f64) == 8, "");

static_assert(sizeof(u8) == sizeof(s8), "");
static_assert(sizeof(u16) == sizeof(s16), "");
static_assert(sizeof(u32) == sizeof(s32), "");
static_assert(sizeof(u64) == sizeof(s64), "");

///////////////////////////////////////////////////////////////////////
// Arena
//

struct arena
{
    u8 *Base;
    u64 Pos;
    u64 Commit;
    u64 Reserve;
    u32 TempCount;
};

struct temp_memory
{
    arena *Arena;
    u64 StartPos;
};

enum arena_push_flags : u8
{
    ArenaPushFlags_None = 0,
    ArenaPushFlags_ClearToZero = 1,
};

arena_push_flags DefaultArenaPushFlags()
{
    arena_push_flags Flags = ArenaPushFlags_ClearToZero;
    return Flags;
}

arena_push_flags NoClear()
{
    arena_push_flags Flags = ArenaPushFlags_None;
    return Flags;
}

void        MemoryCopy(u64 Size, void *Source, void *Dest);
void        MemoryCopyOverlapped(u64 Size, void *Source, void *Dest);
void        MemoryZero(u64 Size, void *Memory);

arena *     CreateArena(u64 ReserveSize);
void        FreeArena(arena *Arena);

void *      PushSize_(arena *Arena, u64 Size, u32 Alignment, u8 ArenaPushFlags = DefaultArenaPushFlags());
void *      PushCopy_(arena *Arena, void *Source, u64 Size, u32 Alignment); 
void        PopToPosition(arena *Arena, u64 Pos);
void        PopSize(arena *Arena, u64 Size);

temp_memory BeginTempMemory(arena *Arena);
void        EndTempMemory(temp_memory TempMemory);
void        CheckArena(arena *Arena);
temp_memory GetScratch();
void        ReleaseScratch(temp_memory ScratchMemory);


// May need to write ##__VA_ARGS__ to remove comma that is placed if varargs is empty
#define Allocate(Arena, Count, Type, ...) (Type *)PushSize_((Arena), (Count)*sizeof(Type), alignof(Type),  ##__VA_ARGS__)

#define PushStruct(Arena, Type, ...) (Type *)PushSize_((Arena), sizeof(Type), alignof(Type),  __VA_ARGS__)

#define PushCopy(Arena, Source, Count, Type) (Type *)PushCopy_((Arena), (void *)(Source), (Count)*sizeof(Type), alignof(Type))

///////////////////////////////////////////////////////////////////////
// Ring Buffer
//

struct ring_buffer
{
    //   r   w
    // | 0123   |
    u8 *Memory;
    // These need to be u64 with the current implementation.
    // If we want to the to be u32 then the Capacity must be a power of two.
    u64 Read;   // Read % Capacity indexes the first byte availble for reading
    u64 Write;  // Write % Capacity indexes the first byte free for writing (one past the last byte for reading)
    u64 Capacity; 
    void *Handle;
};

///////////////////////////////////////////////////////////////////////
// String
//

struct string
{
    u8 *Str;
    u64 Length;
    
    const u8 &operator[](size_t Index) const
    {
        Assert(Index >= 0 && Index < Length);
        return Str[Index];
    }
    u8 &operator[](size_t Index)
    {
        Assert(Index >= 0 && Index < Length);
        return Str[Index];
    }
};

struct string_builder
{
    struct string_node
    {
        string String;
        string_node *Next;
    };
    
    string_node *Head;
    string_node *Tail;
    u64 Length;
    
    void Append(arena *Arena, string NewString);
    void Append(arena *Arena, char *NewString);
    string GetString(arena *Arena);
};

#define StringLit(lit) string{(u8 *)(lit), sizeof(lit) - 1} // Null terminator not included in Length

string MakeString(u8 *CString);
string MakeString(u8 *StringData, u64 Length);
u8 *   PushCString(arena *Arena, u8 *String);
string PushString(arena *Arena, string String);

string PushFormatStringArgs(arena *Arena, char *Format, va_list Args);
string PushFormatString(arena *Arena, char *Format, ...);

u64    StringLength(u8 *CString);
u8     StringGetChar(string String, u64 Index);
bool   IsUpper(u8 c);
bool   IsLower(u8 c);
bool   IsAlpha(u8 c);
bool   IsNumeric(u8 c);
bool   IsAlphaNumeric(u8 c);
bool   IsWhitespace(u8 c);
u8     ToUpper(u8 c);
u8     ToLower(u8 c);
bool   CharIsSlash(u8 c);

void   StringToLower(string *String);
void   StringToUpper(string *String);
void   StringSkip(string *String, u64 N);
void   StringChop(string *String, u64 N);

string StringGetPrefix(string String, u64 N);
string StringGetSuffix(string String, u64 N);

u64    StringCountOccurence(string String, u8 Char);
u64    StringFindChar(string String, u8 Char, u32 Offset=0);
u64    StringFindCharReverse(string String, u8 Char);
bool   StringContainsChar(string String, u8 Char);
bool   StringsAreEqual(string A, string B);
bool   StringsAreEqualCaseInsensitive(string a, string b);
bool   StringContainsSubstr(string String, string Substr);

u64    StringFindLastSlash(string Path);
void   StringRemoveExtension(string *File);
string StringFilenameFromPath(string Path);

string_builder CreateStringBuilder();
