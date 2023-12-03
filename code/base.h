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
// ARENA_GUARD_PAGES  Enables buffer overflow checking for arena allocations. Enabled if BASE_DEBUG is on.

// NOTE: Things to consider
// - Currently the math functions are in the header forcing us to include math.h here. They need to be in the 
//   header if there are to be marked inline. I think I would prefer they be in the .cpp file. We probably need
//   a cpp file because we need to include platform.h somewhere and it is maybe cleaner than having a #define 
//   BASE_IMPLEMENTATION situation.
// - Maybe rename platform to OS, which is shorter and a better prefix for types.

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

// Check clang first because clang often will define __GNUC__ or _MSC_VER as well
#if defined(__clang__)
#define BASE_COMPILER_CLANG 1
#elif defined(_MSC_VER)
#define BASE_COMPILER_MSVC 1
#elif defined(__GNUC__)
#define BASE_COMPILER_GCC 1
#else
#error "Could not detect compiler"
#endif

#if defined(_M_IX86) || defined(__i386__) 
#define BASE_ARCH_X86 1              // 32-bit x86 architecture
#elif defined(_M_X64) || defined(__x86_64__) 
#define BASE_ARCH_X64 1              // 64-bit x86 architecture
#elif defined(__arm__) 
#define BASE_ARCH_ARM 1              
#else
#error "Could not detect CPU architecture"
#endif

#if !BASE_OS_WINDOWS && !BASE_OS_LINUX
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

#if !defined(ARENA_GUARD_PAGES) && (BASE_DEBUG == 1)
#define ARENA_GUARD_PAGES 1
#endif

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

enum arena_push_flags : u32
{
    ArenaPushFlags_None = 0,
    ArenaPushFlags_ClearToZero = 1,
};

u32 ClearToZero()
{
    u32 Flags = ArenaPushFlags_ClearToZero;
    return Flags;
}

u32 NoClear()
{
    u32 Flags = ArenaPushFlags_None;
    return Flags;
}

void        MemoryCopy(u64 Size, void *Source, void *Dest);
void        MemoryCopyOverlapped(u64 Size, void *Source, void *Dest);
void        MemoryZero(u64 Size, void *Memory);
bool        MemoryIsEqual(u64 Size, void *A, void *B);

bool        IsPowerOfTwo(u64 x);
u64         AlignUp(u64 Value, u64 Align);

arena *     CreateArena(u64 ReserveSize);
void        FreeArena(arena *Arena);

void *      PushSize_(arena *Arena, u64 Size, u32 Alignment, u32 ArenaPushFlags = ArenaPushFlags_ClearToZero);
void *      PushCopy_(arena *Arena, u64 Size, void *Source, u32 Alignment); 
void        PopToPosition(arena *Arena, u64 Pos);
void        PopSize(arena *Arena, u64 Size);
void        ClearArena(arena *Arena);

temp_memory BeginTempMemory(arena *Arena);
void        EndTempMemory(temp_memory TempMemory);
void        CheckArena(arena *Arena);
temp_memory GetScratch();
void        ReleaseScratch(temp_memory ScratchMemory);

// ## __VA_ARGS is an extension on gcc and clang, and the MSVC preprocessor will eat trailing commas when 
// VA_ARGS is empty. But this will generate warnings with pedantic error checking.
//#define PushArray(Arena, Count, Type, ...) (Type *)PushSize_((Arena), (Count)*sizeof(Type), alignof(Type), ##__VA_ARGS__)
//#define PushStruct(Arena, Type, ...) (Type *)PushSize_((Arena), sizeof(Type), alignof(Type) ##__VA_ARGS__)

#define PushArray(Arena, Count, Type) (Type *)PushSize_((Arena), (Count)*sizeof(Type), alignof(Type), ArenaPushFlags_None)
#define PushArrayZero(Arena, Count, Type) (Type *)PushSize_((Arena), (Count)*sizeof(Type), alignof(Type), ArenaPushFlags_ClearToZero)
#define PushStruct(Arena, Type) (Type *)PushSize_((Arena), sizeof(Type), alignof(Type), ArenaPushFlags_ClearToZero)

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

struct string_list
{
    struct string_node
    {
        string String;
        string_node *Next;
    };
    
    string_node *Head;
    string_node *Tail;
    u64 Length;
};

#define StringLit(lit) string{(u8 *)(lit), sizeof(lit) - 1} // Null terminator not included in Length

string CreateString(u8 *CString);
string CreateString(u8 *StringData, u64 Length);

string PushString(arena *Arena, string String);
u8 *   PushCString(arena *Arena, u8 *String);
u8 *   PushCString(arena *Arena, string String);
string PushStringfArgs(arena *Arena, char *Format, va_list Args);
string PushStringf(arena *Arena, char *Format, ...);

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
bool   StringsAreEqual(string A, string B);
bool   StringsAreEqualCaseInsensitive(string a, string b);
bool   StringContainsSubstr(string String, string Substr);

u64    FindLastSlash(string Path);
void   RemoveExtension(string *File);
string FilenameFromPath(string Path);
string DirectoryFromPath(string Path);

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

struct v2s
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
