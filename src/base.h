#pragma once

#include <stdint.h>
#include <float.h> 
#include <math.h>  
#include <stdarg.h> // va_list

///////////////////////////////////////////////////////////////////////
// Usage
//

// Flags:
// BASE_DEBUG         Enables asserts. Enabled by default.

// NOTE: Things to consider
// - Currently the math functions are in the header forcing us to include math.h here. They need to be in the 
//   header if there are to be marked inline. I think I would prefer they be in the .cpp file. We probably need
//   a cpp file because we need to include os.h somewhere and it is maybe cleaner than having a #define 
//   BASE_IMPLEMENTATION situation.

///////////////////////////////////////////////////////////////////////
// Platform detection
//

#if defined(_WIN32)
#  define BASE_OS_WINDOWS 1
#elif defined(__linux__)
#  define BASE_OS_LINUX 1
#elif defined(__APPLE__) || defined(__MACH__)
#  define BASE_OS_MAC 1
#elif defined(__unix__)
#  define BASE_OS_UNIX 1               // Some non-linux non-apple unix variety
#else
#  error "Could not detect operating system"
#endif

#if !defined(BASE_OS_WINDOWS)
#  define BASE_OS_WINDOWS 0
#endif
#if !defined(BASE_OS_LINUX)
#  define BASE_OS_LINUX 0
#endif
#if !defined(BASE_OS_MAC)
#  define BASE_OS_MAC 0
#endif
#if !defined(BASE_OS_UNIX)
#  define BASE_OS_UNIX 0
#endif

// Check clang first because clang often will define __GNUC__ or _MSC_VER as well
#if defined(__clang__)
#  define BASE_COMPILER_CLANG 1
#elif defined(_MSC_VER)
#  define BASE_COMPILER_MSVC 1
#elif defined(__GNUC__)
#  define BASE_COMPILER_GCC 1
#else
#  error "Could not detect compiler"
#endif

#if !defined(BASE_COMPILER_CLANG)
#  define BASE_COMPILER_CLANG 0
#endif
#if !defined(BASE_COMPILER_MSVC)
#  define BASE_COMPILER_MSVC 0
#endif
#if !defined(BASE_COMPILER_GCC)
#  define BASE_COMPILER_GCC 0
#endif

#if defined(_M_IX86) || defined(__i386__) 
#  define BASE_ARCH_X86 1              // 32-bit x86 architecture
#elif defined(_M_X64) || defined(__x86_64__) 
#  define BASE_ARCH_X64 1              // 64-bit x86 architecture
#elif defined(__arm__) 
#  define BASE_ARCH_ARM 1              
#else
#  error "Could not detect CPU architecture"
#endif

#if !defined(BASE_ARCH_X86)
#  define BASE_ARCH_X86 0
#endif
#if !defined(BASE_ARCH_X64)
#  define BASE_ARCH_X64 0
#endif
#if !defined(BASE_ARCH_ARM)
#  define BASE_ARCH_ARM 0
#endif

#if !BASE_OS_WINDOWS && !BASE_OS_LINUX
#  error "This operating system is currently not supported"
#endif

///////////////////////////////////////////////////////////////////////
// Assert
//

#if !defined(BASE_DEBUG_TRAP)

#  if BASE_OS_WINDOWS
//// On windows use __debugbreak
extern void __cdecl __debugbreak(void); // To avoid including intrin.h
#    define BASE_DEBUG_TRAP() __debugbreak() 
#  elif BASE_OS_MAC
#    error "Not implemented"
#  elif BASE_ARCH_ARM
#    error "Not implemented"
#  elif BASE_ARCH_X64 || BASE_ARCH_X86
#    define BASE_DEBUG_TRAP() __builtin_trap()
#  else
#    define BASE_DEBUG_TRAP() (*(int *)0 = 0;)
#  endif
#endif

#if BASE_COMPILER_CLANG || BASE_COMPILER_GCC
// 1 based indexing for parameter
#  define BASE_FORMAT_STRING_CHECK(StringIndex, ArgsIndex)  __attribute__((__format__ (__printf__, StringIndex, ArgsIndex)))
#else
#  define BASE_FORMAT_STRING_CHECK(StringIndex, ArgsIndex) 
#endif

#define BASE_STRINGIFY_(Str) #Str
#define BASE_STRINGIFY(Str) BASE_STRINGIFY_(Str)

#define CONCAT_(x, y) x##y // Can concat literal characters xy if x and y are macro definitions
#define CONCAT(x, y) CONCAT_(x, y) // So macro expand x and y first

// Handles [0, 100] arguments
// Relies on GCC extension ##__VA_ARGS (which works in MSVC with the new preprocessor implementation)
#define _NUM_ARGS(dummy, X100, X99, X98, X97, X96, X95, X94, X93, X92, X91, X90, X89, X88, X87, X86, X85, X84, X83, X82, X81, X80, X79, X78, X77, X76, X75, X74, X73, X72, X71, X70, X69, X68, X67, X66, X65, X64, X63, X62, X61, X60, X59, X58, X57, X56, X55, X54, X53, X52, X51, X50, X49, X48, X47, X46, X45, X44, X43, X42, X41, X40, X39, X38, X37, X36, X35, X34, X33, X32, X31, X30, X29, X28, X27, X26, X25, X24, X23, X22, X21, X20, X19, X18, X17, X16, X15, X14, X13, X12, X11, X10, X9, X8, X7, X6, X5, X4, X3, X2, X1, N, ...) N

#define NUM_ARGS(...) _NUM_ARGS(dummy, ##__VA_ARGS__, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

static_assert(NUM_ARGS() == 0, "Empty argument must be zero");
static_assert(NUM_ARGS(A) == 1, "Must be 1");
static_assert(NUM_ARGS(A, B) == 2, "Must be 2");

// If BASE_DEBUG is not defined then we enable it, 
// If BASE_DEBUG is defined but with no value then we still get assertions, 
// if BASE_DEBUG is zero then we don't have assertions (unless they are specifically turned on), 
// if BASE_DEBUG is on then we have assertions (unless they are specifically turned off).
#if !defined(BASE_DEBUG)
#  define BASE_DEBUG 1
#endif

void BaseAssertPrint_(const char *Format, ...);

#if (defined(BASE_ENABLE_ASSERT) && (BASE_ENABLE_ASSERT == 0)) || (!defined(BASE_ENABLE_ASSERT) && BASE_DEBUG == 0)
#  define Assert(Cond) ((void)0)
#else
// In this instance we don't want to macro expand Cond, so that we can print our macros without substitution.
#  define Assert(Cond) \
do { \
if (!static_cast<bool>((Cond))) { \
BaseAssertPrint_("%s:%s():%i: Assertion: `%s' failed.\n", __FILE__, __func__, __LINE__, #Cond); \
BASE_DEBUG_TRAP(); \
} \
} while(0) 

#endif

///////////////////////////////////////////////////////////////////////
// Helpers
//

#define Minimum(a, b) ((a) < (b) ? (a) : (b))
#define Maximum(a, b) ((a) > (b) ? (a) : (b))
#define Clamp(Value, Low, High) (((Value) < (Low)) ? (Low) : ((Value) > (High)) ? (High) : (Value))
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

#define SLL_APPEND(Head, Tail, Node) ((Tail) = (Tail) ? ((Tail)->Next = Node) : ((Head) = Node))

#define DLL_PUSH_TAIL(Head, Tail, Node) ((Tail) = ((Tail) ? (Node)->Prev = (Tail), (Tail)->Next = (Node) : (Head) = (Node)))
#define DLL_POP_TAIL(Head, Tail) ((Tail) == (Head) ? (Tail) = 0, (Head) = 0 : (Tail) = (Tail)->Prev)

#define DECIMAL_DIGIT_COUNT_MAX(type) (241 * sizeof(type) / 100 + 1)


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

// From sanitizer/asan_interface.h
#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
#  define ASAN_ENABLED 1
#else
#  define ASAN_ENABLED 0
#endif

#if ASAN_ENABLED
extern "C" {
    void __asan_poison_memory_region(void const volatile *addr, size_t size);
    void __asan_unpoison_memory_region(void const volatile *addr, size_t size);
}
#  define ASAN_POISON_MEMORY_REGION(addr, size) __asan_poison_memory_region((addr), (size))
#  define ASAN_UNPOISON_MEMORY_REGION(addr, size) __asan_unpoison_memory_region((addr), (size))
#else
#  define ASAN_POISON_MEMORY_REGION(addr, size) ((void)(addr), (void)(size))
#  define ASAN_UNPOISON_MEMORY_REGION(addr, size) ((void)(addr), (void)(size))
#endif 

struct arena
{
    u8 *Base;
    u64 Pos;
    u64 Commit;
    u64 Reserve;
    u32 TempCount; // I'm not sure if this is useful
};

struct temp_arena
{
    arena *Arena;
    u64 StartPos;
    u32 TempIndex;
};

enum arena_push_flags : u32
{
    ArenaPushFlags_None = 0,
    ArenaPushFlags_ClearToZero = 1,
};

void        MemoryCopy(u64 Size, void *Source, void *Dest);
void        MemoryCopyOverlapped(u64 Size, void *Source, void *Dest);
void        MemoryZero(u64 Size, void *Memory);
bool        MemoryCompare(u64 Size, void *A, void *B);

bool        IsPowerOfTwo(u64 x);
u64         AlignUp(u64 Value, u64 Align);

arena *     CreateArena(u64 ReserveSize);
void        FreeArena(arena *Arena);

void *      PushSize_(arena *Arena, u64 Size, u32 Alignment, u32 ArenaPushFlags = ArenaPushFlags_ClearToZero);
void *      PushCopy_(arena *Arena, u64 Size, void *Source, u32 Alignment); 
void        PopToPosition(arena *Arena, u64 Pos);
void        PopSize(arena *Arena, u64 Size);
void        ClearArena(arena *Arena);

temp_arena  BeginTempArena(arena *Arena);
void        EndTempArena(temp_arena TempArena);
void        CheckArena(arena *Arena);
void        ReleaseScratch(temp_arena ScratchMemory);

struct conflicting_arena_array
{
    arena *Array[2];
};

temp_arena  GetScratchImpl(u32 ConflictCount, conflicting_arena_array Conflicts);
#define GetScratch(...) GetScratchImpl(NUM_ARGS(__VA_ARGS__), conflicting_arena_array{__VA_ARGS__})

#define PushArray(Arena, Count, Type) (Type *)PushSize_((Arena), (Count)*sizeof(Type), alignof(Type), ArenaPushFlags_ClearToZero)
#define PushArrayNoZero(Arena, Count, Type) (Type *)PushSize_((Arena), (Count)*sizeof(Type), alignof(Type), ArenaPushFlags_None)
#define PushStruct(Arena, Type) (Type *)PushSize_((Arena), sizeof(Type), alignof(Type), ArenaPushFlags_ClearToZero)
#define PushCopy(Arena, Count, Type, Source) (Type *)PushCopy_((Arena), ((Count) * sizeof(Type)), (Source), alignof(Type))

///////////////////////////////////////////////////////////////////////
// String
//

struct string
{
    u8 *Str;
    u64 Length;
    
    const u8 &operator[](size_t Index) const
    {
        Assert(Index < Length);
        return Str[Index];
    }
    u8 &operator[](size_t Index)
    {
        Assert(Index < Length);
        return Str[Index];
    }
};

struct string_node
{
    string String;
    string_node *Next;
};

struct string_list
{
    string_node *Head;
    string_node *Tail;
    u64 Length;
    u64 Count;
};

// We could make a StrLitStruct that does a case of memory to u8 * and uses sizeof struct
// this lets you use StringListJoin on structs.
#define Strlit(lit) string{(u8 *)(lit), sizeof(lit) - 1} 
#define StrVal(String) (int)(String).Length, (String).Str

string CreateString(u8 *CString);
string CreateString(u8 *StringData, u64 Length);

string PushString(arena *Arena, string String);
u8 *   PushCString(arena *Arena, u8 *String);
u8 *   PushCString(arena *Arena, string String);
string PushStringfArgs(arena *Arena, char *Format, va_list Args);\

string PushStringf(arena *Arena, char *Format, ...) BASE_FORMAT_STRING_CHECK(2, 3);

u64    StringLength(u8 *CString);
u8     StringGetChar(string String, u64 Index);
bool   IsUpper(u8 c);
bool   IsLower(u8 c);
bool   IsAlpha(u8 c);
bool   IsNumber(u8 c);
bool   IsAlphaNumeric(u8 c);
bool   IsWhitespace(u8 c);
bool   IsSlash(u8 c);
u8     ToUpper(u8 c);
u8     ToLower(u8 c);

string StringPrefix(string String, u64 N);
string StringSuffix(string String, u64 N);
string StringSkip(string String, u64 N);
string StringChop(string String, u64 N);
string SubstrRange(string String, u64 Start, u64 OnePastEnd);
string Substr(string String, u64 Start, u64 Length);

void   StringToLower(string *String);
void   StringToUpper(string *String);

u64    StringCountOccurence(string String, u8 Char);
u64    StringFindChar(string String, u8 Char, u32 Offset=0);
u64    StringFindLastChar(string String, u8 Char);
u64    StringFindStr(string Haystack, string Needle);
bool   StringContainsChar(string String, u8 Char);
bool   StringContainsStr(string String, string Substr);
bool   StringsAreEqual(string A, string B);
bool   StringsAreEqualCaseInsensitive(string a, string b);

u64    StringFindLastSlash(string Path);
void   RemoveExtension(string *File);
string FilenameFromPath(string Path);
string DirectoryFromPath(string Path);

void StringListAppend(arena *Arena, string_list *List, string String);
void StringListAppend(arena *Arena, string_list *List, char *String);
string StringListJoin(arena *Arena, string_list *List);
string_list StringListSplit(arena *Arena, string String, string Splits);
template<typename... Ts> string StringConcat(arena *Arena, Ts... args);


struct parsed_num
{
    u64 Value;
    bool Error;
};

parsed_num StringParseNum(string String);

///////////////////////////////////////////////////////////////////////
// Math
//

// TODO: integer vector, interval types ({min, max} union {p0, p1} union {x0, x1, y0, x1})

constexpr s8  S8Max  = 127;
constexpr s8  S8Min  = -S8Max - 1;
constexpr s16 S16Max = 32767;
constexpr s16 S16Min = -S16Max - 1;
constexpr s32 S32Max = 2147483647;
constexpr s32 S32Min = -S32Max - 1;
constexpr s64 S64Max = 9223372036854775807;
constexpr s64 S64Min = -S64Max - 1;

constexpr u8  U8Max  = 0xFF;
constexpr u16 U16Max = 0xFFFF;
constexpr u32 U32Max = 0xFFFFFFFF;
constexpr u64 U64Max = 0xFFFFFFFFFFFFFFFF;

constexpr f32 F32Max = FLT_MAX;
constexpr f32 F32Min = -FLT_MAX;  // smallest negative number != FLT_MIN 
constexpr f64 F64Max = DBL_MAX;
constexpr f64 F64Min = -DBL_MAX; 
constexpr f32 Pi32   = 3.14159265359f;
constexpr f32 Tau32  = 6.28318530717958647692f;

// Anonymous structs are a compiler extension, and will generate warnings
// Anonymous unions are allowed, however you need anonymous structs to get simple swizzling.

union v2
{
    struct { f32 x, y; };
    struct { f32 u, v; };
    struct { f32 Width, Height; };
    f32 E[2];
};

union v2s
{
    struct { s32 x, y; };
    struct { s32 Width, Height; };
    f32 E[2];
};

union v3
{
    struct { f32 x, y, z; };
    struct { f32 r, g, b; };
    struct { v2  xy; f32 Unused0_; };
    struct { f32 Unused1_; v2 yz; };
    f32 E[3];
};

union v4
{
    struct
    {
        union
        {
            v3 xyz;
            struct { f32 x, y, z; };
        };
        f32 w;
    };
    struct
    {
        union
        {
            v3 rgb;
            struct { f32 r, g, b; };
        };
        
        f32 a;
    };
    struct
    {
        v2 xy;
        f32 Unused0_;
        f32 Unused1_;
    };
    struct
    {
        f32 Unused2_;
        v2 yz;
        f32 Unused3_;
    };
    struct
    {
        f32 Unused4_;
        f32 Unused5_;
        v2 zw;
    };
    f32 E[4];
};

struct m4x4
{
    // Stored in Row-Major (basis vectors are not contiguous like OpenGL expects)
    // Initialised so that floats can be written in 'column-major' order.
    // e.g.
    //   xvec  yvec  zvec  wvec
    // { 1.0f, 0.0f, 0.0f, 0.0f,
    //   0.0f, 1.0f, 0.0f, 0.0f,
    //   0.0f, 0.0f, 1.0f, 0.0f,
    //   0.0f, 0.0f, 0.0f, 1.0f };
    f32 E[4][4];
};

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

inline f32 ACos(f32 X)
{
    return acosf(X);
}

inline f32 ATan2(f32 Y, f32 X)
{
    return atan2f(Y, X);
}

inline f32 Square(f32 X)
{
    return X * X;
}

inline f32 SquareRoot(f32 X)
{
    return sqrtf(X);
}

inline f32 Pow(f32 Base, f32 Exponent)
{
    return powf(Base, Exponent);
}

inline f32 AbsoluteValue(f32 X)
{
    return fabsf(X);
}

inline f32 Round(f32 X)
{
    return roundf(X);
}

inline f32 Floor(f32 X)
{
    return floorf(X);
}

inline f32 Lerp(f32 A, f32 t, f32 B)
{
    return (1.0f - t)*A + t*B;
}

inline f32 Clamp01(f32 Value)
{
    return Clamp(Value, 0.0f, 1.0f);
}

constexpr f32 RadFromDeg(f32 Degrees)
{
    return Degrees * 0.01745329251994329576923690768489f;
}

constexpr f32 DegFromRad(f32 Radians)
{
    return Radians * 57.295779513082320876798154814105f;
}

inline u32 RoundToU32(f32 X)
{
    Assert(X > -0.5f);
    u32 result = (u32)roundf(X);
    return result;
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

inline v2s V2S(s32 x, s32 y)
{
    return v2s{x, y};
}

inline v2 operator+(v2 A, v2 B)          { return v2{A.x+B.x, A.y+B.y}; } 
inline v2 operator-(v2 A, v2 B)          { return v2{A.x-B.x, A.y-B.y}; } 
inline v2 operator-(v2 A)                { return v2{-A.x,   -A.y}; }     

inline v2 operator*(f32 Scalar, v2 A)    { return v2{Scalar*A.x, Scalar*A.y}; }     
inline v2 operator/(f32 Scalar, v2 A)    { return v2{Scalar/A.x, Scalar/A.y}; }
inline v2 operator*(v2 A, f32 Scalar)    { return Scalar * A; }
inline v2 operator/(v2 A, f32 Scalar)    { return (1.0f / Scalar) * A; } 

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

inline v2s operator+(v2s A, v2s B)       { return v2s{A.x+B.x, A.y+B.y}; } 
inline v2s operator-(v2s A, v2s B)       { return v2s{A.x-B.x, A.y-B.y}; } 
inline v2s operator-(v2s A)              { return v2s{-A.x,   -A.y}; }           

inline v2s& operator+=(v2s& A, v2s B)    { return (A = A + B); }
inline v2s& operator-=(v2s& A, v2s B)    { return (A = A - B); }


inline f32 Inner(v2 A, v2 B)
{
    return A.x*B.x + A.y*B.y;
}

inline v2 PerpCCW(v2 A)
{
    return v2{-A.y, A.x};
}

inline v2 PerpCW(v2 A)
{
    return v2{A.y, -A.x};
}

inline v2 Perp(v2 A)
{
    return PerpCCW(A);
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

static v4 operator*(m4x4 M, v4 V)
{
    v4 Result {
        M.E[0][0] * V.x + M.E[0][1] * V.y + M.E[0][2] * V.z + M.E[0][3] * V.w,
        M.E[1][0] * V.x + M.E[1][1] * V.y + M.E[1][2] * V.z + M.E[1][3] * V.w,
        M.E[2][0] * V.x + M.E[2][1] * V.y + M.E[2][2] * V.z + M.E[2][3] * V.w,
        M.E[3][0] * V.x + M.E[3][1] * V.y + M.E[3][2] * V.z + M.E[3][3] * V.w,
    };
    
    return Result;
}

static m4x4 operator*(m4x4 A, m4x4 B)
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

inline m4x4 Identity()
{
    return m4x4 {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
};

inline m4x4 RotateZ(f32 Angle)
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

inline m4x4 RotateX(f32 Angle)
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

inline m4x4 RotateY(f32 Angle)
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

inline m4x4 Scale(f32 S)
{
    return m4x4 {
        S, 0.0f, 0.0f, 0.0f,
        0.0f, S, 0.0f, 0.0f,
        0.0f, 0.0f, S, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

inline m4x4 Translation(v3 T)
{
    return m4x4 {
        1.0f, 0.0f, 0.0f, T.x,
        0.0f, 1.0f, 0.0f, T.y,
        0.0f, 0.0f, 1.0f, T.z,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

static m4x4 LookAt(v3 CameraP, v3 TargetP, v3 WorldUp)
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

inline m4x4 PerspectiveProjection(f32 AspectWidthOverHeight, f32 FocalLength, f32 NearClipPlane, f32 FarClipPlane)
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

inline m4x4 OrthographicProjection3D(f32 Left, f32 Right, f32 Bottom, f32 Top, f32 Near, f32 Far)
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

inline m4x4 OrthographicProjection2D(f32 Left, f32 Right, f32 Bottom, f32 Top)
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

///////////////////////////////////////////////////////////////////////
// Error Messages
//

struct error_msg {
    error_msg *Next;
    error_msg *Prev;
    u64 ArenaPos;
    string Message;
};
