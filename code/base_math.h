#ifndef DRAW_3D_MATH_H
#define DRAW_3D_MATH_H

#include <math.h>
#include <float.h>

#include "base.h"

#define R32Max FLT_MAX
#define R32Min -FLT_MAX
#define Pi32 3.14159265359f
#define Tau32 6.28318530717958647692f

union v2
{
    struct
    {
        f32 x, y;
    };
    struct
    {
        f32 u, v;
    };
    struct
    {
        f32 width, Height;
    };
    f32 E[2];
};

union v3
{
    struct
    {
        f32 x, y, z;
    };
    struct
    {
        f32 u, v, __;
    };
    struct
    {
        f32 r, g, b;
    };
    struct
    {
        v2 xy;
        f32 ignored0_;
    };
    struct
    {
        f32 ignored1_;
        v2 yz;
    };
    struct
    {
        v2 uv;
        f32 ignored2_;
    };
    struct
    {
        f32 ignored3_;
        v2 v__;
    };
    f32 E[3];
};

union v4
{
    struct
    {
        union
        {
            v3 xyz;
            struct
            {
                f32 x, y, z;
            };
        };
        
        f32 w;
    };
    struct
    {
        union
        {
            v3 rgb;
            struct
            {
                f32 r, g, b;
            };
        };
        
        f32 a;
    };
    struct
    {
        v2 xy;
        f32 ignored0_;
        f32 ignored1_;
    };
    struct
    {
        f32 ignored2_;
        v2 yz;
        f32 ignored3_;
    };
    struct
    {
        f32 ignored4_;
        f32 ignored5_;
        v2 zw;
    };
    f32 E[4];
};


inline v2
V2(f32 x, f32 y)
{
    v2 result;
    
    result.x = x;
    result.y = y;
    
    return result;
}

inline v3
V3(f32 x, f32 y, f32 z)
{
    v3 result;
    
    result.x = x;
    result.y = y;
    result.z = z;
    
    return result;
}

inline v3
V3(v2 xy, f32 z)
{
    v3 result;
    
    result.x = xy.x;
    result.y = xy.y;
    result.z = z;
    
    return result;
}

inline v4
V4(f32 x, f32 y, f32 z, f32 w)
{
    v4 result;
    
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    
    return result;
}

inline v4
V4(v3 xyz, f32 w)
{
    v4 result;
    
    result.xyz = xyz;
    result.w = w;
    
    return result;
}

//////////////////////////////// Operations

inline s32
CeilToInt32(f32 A)
{
    s32 result = (s32)ceilf(A);
    return result;
}

inline s64
RoundToInt64(f32 A)
{
    s64 result = (s64)llroundf(A);
    return result;
}

inline u32
RoundToUInt32(f32 A)
{
    Assert(A > -0.5f);
    u32 result = (u32)roundf(A);
    return result;
}

inline f32
Round(f32 A)
{
    f32 result = (f32)roundf(A);
    return result;
}

inline f32
Floor(f32 A)
{
    f32 result = (f32)floorf(A);
    return result;
}

inline f32
Square(f32 A)
{
    f32 result = A*A;
    return result;
}

inline f32
SquareRoot(f32 A)
{
    f32 result = sqrtf(A);
    return result;
}

f32 
Pow(f32 A, f32 B)
{
    f32 result = pow(A, B);
    return result;
}

inline f32
AbsoluteValue(f32 A)
{
    f32 result = fabsf(A);
    return result;
}

inline f32
Lerp(f32 A, f32 t, f32 B)
{
    f32 result = (1.0f - t)*A + t*B;
    return result;
}

inline f32
Clamp01(f32 Value)
{
    f32 result = Clamp(0.0f, Value, 1.0f);
    
    return result;
}


inline v2
Perp(v2 A)
{
    v2 result = {-A.y, A.x};
    return result;
}

inline v2
operator*(f32 A, v2 B)
{
    v2 result;
    
    result.x = A*B.x;
    result.y = A*B.y;
    
    return result;
}

inline v2
operator*(v2 B, f32 A)
{
    v2 result = A*B;
    
    return result;
}

inline v2 &
operator*=(v2 &B, f32 A)
{
    B = A * B;
    
    return(B);
}

inline v2
operator-(v2 A)
{
    v2 result;
    
    result.x = -A.x;
    result.y = -A.y;
    
    return result;
}

inline v2
operator+(v2 A, v2 B)
{
    v2 result;
    
    result.x = A.x + B.x;
    result.y = A.y + B.y;
    
    return result;
}

inline v2 &
operator+=(v2 &A, v2 B)
{
    A = A + B;
    
    return(A);
}

inline v2
operator-(v2 A, v2 B)
{
    v2 result;
    
    result.x = A.x - B.x;
    result.y = A.y - B.y;
    
    return result;
}

inline v2 &
operator-=(v2 &A, v2 B)
{
    A = A - B;
    
    return(A);
}

inline v2
Hadamard(v2 A, v2 B)
{
    v2 result = {A.x*B.x, A.y*B.y};
    
    return result;
}

inline f32
Inner(v2 A, v2 B)
{
    f32 result = A.x*B.x + A.y*B.y;
    
    return result;
}

inline f32
LengthSq(v2 A)
{
    f32 result = Inner(A, A);
    
    return result;
}

inline f32
Length(v2 A)
{
    f32 result = SquareRoot(LengthSq(A));
    return result;
}

inline v2
Clamp01(v2 Value)
{
    v2 result;
    
    result.x = Clamp01(Value.x);
    result.y = Clamp01(Value.y);
    
    return result;
}

inline v3
operator*(f32 A, v3 B)
{
    v3 result;
    
    result.x = A*B.x;
    result.y = A*B.y;
    result.z = A*B.z;
    
    return result;
}

inline v3
operator*(v3 B, f32 A)
{
    v3 result = A*B;
    
    return result;
}

inline v3 &
operator*=(v3 &B, f32 A)
{
    B = A * B;
    
    return(B);
}

inline v3
operator/(v3 B, f32 A)
{
    v3 result = (1.0f/A)*B;
    
    return result;
}

inline v3
operator/(f32 B, v3 A)
{
    v3 result =
    {
        B / A.x,
        B / A.y,
        B / A.z,
    };
    
    return result;
}

inline v3 &
operator/=(v3 &B, f32 A)
{
    B = B / A;
    
    return(B);
}

inline v3
operator-(v3 A)
{
    v3 result;
    
    result.x = -A.x;
    result.y = -A.y;
    result.z = -A.z;
    
    return result;
}

inline v3
operator+(v3 A, v3 B)
{
    v3 result;
    
    result.x = A.x + B.x;
    result.y = A.y + B.y;
    result.z = A.z + B.z;
    
    return result;
}

inline v3 &
operator+=(v3 &A, v3 B)
{
    A = A + B;
    
    return(A);
}

inline v3
operator-(v3 A, v3 B)
{
    v3 result;
    
    result.x = A.x - B.x;
    result.y = A.y - B.y;
    result.z = A.z - B.z;
    
    return result;
}

inline v3 &
operator-=(v3 &A, v3 B)
{
    A = A - B;
    
    return(A);
}

#if 0
inline v3
Hadamard(v3 A, v3 B)
{
    v3 result = {A.x*B.x, A.y*B.y, A.z*B.z};
    
    return result;
}
#endif

inline f32
Inner(v3 A, v3 B)
{
    f32 result = A.x*B.x + A.y*B.y + A.z*B.z;
    
    return result;
}

inline v3
Cross(v3 A, v3 B)
{
    v3 result;
    
    result.x = A.y*B.z - A.z*B.y;
    result.y = A.z*B.x - A.x*B.z;
    result.z = A.x*B.y - A.y*B.x;
    
    return result;
}

inline f32
LengthSq(v3 A)
{
    f32 result = Inner(A, A);
    
    return result;
}

inline f32
Length(v3 A)
{
    f32 result = SquareRoot(LengthSq(A));
    return result;
}

inline v3
Normalize(v3 A)
{
    v3 result = A * (1.0f / Length(A));
    
    return result;
}

inline v2
Normalize(v2 A)
{
    v2 result = A * (1.0f / Length(A));
    
    return result;
}

inline v3
NOZ(v3 A)
{
    v3 result = {};
    
    f32 LenSq = LengthSq(A);
    if(LenSq > Square(0.0001f))
    {
        result = A * (1.0f / SquareRoot(LenSq));
    }
    
    return result;
}

inline v3
Clamp01(v3 Value)
{
    v3 result;
    
    result.x = Clamp01(Value.x);
    result.y = Clamp01(Value.y);
    result.z = Clamp01(Value.z);
    
    return result;
}

inline v3
Lerp(v3 A, f32 t, v3 B)
{
    v3 result = (1.0f - t)*A + t*B;
    
    return result;
}

inline v3
Min(v3 A, v3 B)
{
    v3 result =
    {
        Minimum(A.x, B.x),
        Minimum(A.y, B.y),
        Minimum(A.z, B.z),
    };
    
    return result;
}

inline v3
Max(v3 A, v3 B)
{
    v3 result =
    {
        Maximum(A.x, B.x),
        Maximum(A.y, B.y),
        Maximum(A.z, B.z),
    };
    
    return result;
}

inline v4
operator*(f32 A, v4 B)
{
    v4 result;
    
    result.x = A*B.x;
    result.y = A*B.y;
    result.z = A*B.z;
    result.w = A*B.w;
    
    return result;
}

inline v4
operator*(v4 B, f32 A)
{
    v4 result = A*B;
    return result;
}

inline v4 &
operator*=(v4 &B, f32 A)
{
    B = A * B;
    return(B);
}

inline v4
operator/(v4 B, f32 A)
{
    v4 result = (1.0f/A)*B;
    
    return result;
}

inline v4
operator/(f32 B, v4 A)
{
    v4 result =
    {
        B / A.x,
        B / A.y,
        B / A.z,
        B / A.w,
    };
    
    return result;
}

inline v4 &
operator/=(v4 &B, f32 A)
{
    B = B / A;
    
    return(B);
}

inline v4
operator-(v4 A)
{
    v4 result;
    
    result.x = -A.x;
    result.y = -A.y;
    result.z = -A.z;
    result.w = -A.w;
    
    return result;
}

inline v4
operator+(v4 A, v4 B)
{
    v4 result;
    
    result.x = A.x + B.x;
    result.y = A.y + B.y;
    result.z = A.z + B.z;
    result.w = A.w + B.w;
    
    return result;
}

inline v4 &
operator+=(v4 &A, v4 B)
{
    A = A + B;
    return(A);
}

inline v4
operator-(v4 A, v4 B)
{
    v4 result;
    
    result.x = A.x - B.x;
    result.y = A.y - B.y;
    result.z = A.z - B.z;
    result.w = A.w - B.w;
    
    return result;
}

inline v4 &
operator-=(v4 &A, v4 B)
{
    A = A - B;
    return(A);
}

inline v4
Hadamard(v4 A, v4 B)
{
    v4 result = {A.x*B.x, A.y*B.y, A.z*B.z, A.w*B.w};
    return result;
}

inline f32
Inner(v4 A, v4 B)
{
    f32 result = A.x*B.x + A.y*B.y + A.z*B.z + A.w*B.w;
    return result;
}

inline f32
LengthSq(v4 A)
{
    f32 result = Inner(A, A);
    return result;
}

inline f32
Length(v4 A)
{
    f32 result = SquareRoot(LengthSq(A));
    return result;
}

inline v4
Clamp01(v4 Value)
{
    v4 result;
    
    result.x = Clamp01(Value.x);
    result.y = Clamp01(Value.y);
    result.z = Clamp01(Value.z);
    result.w = Clamp01(Value.w);
    
    return result;
}

inline v4
Lerp(v4 A, f32 t, v4 B)
{
    v4 result = (1.0f - t)*A + t*B;
    return result;
}


constexpr f32 RadFromDeg(f32 Degrees)
{
    return Degrees * 0.01745329251994329576923690768489;
}

#endif //DRAW_3D_MATH_H
