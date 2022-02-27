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
    //f32 result = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(A)));
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


inline u32
RGBAPack4x8(v4 Unpacked)
{
    u32 result = ((RoundToUInt32(Unpacked.a) << 24) |
                  (RoundToUInt32(Unpacked.b) << 16) |
                  (RoundToUInt32(Unpacked.g) << 8) |
                  (RoundToUInt32(Unpacked.r) << 0));
    
    return result;
}

inline u32
BGRAPack4x8(v4 Unpacked)
{
    u32 result = ((RoundToUInt32(Unpacked.a) << 24) |
                  (RoundToUInt32(Unpacked.r) << 16) |
                  (RoundToUInt32(Unpacked.g) << 8) |
                  (RoundToUInt32(Unpacked.b) << 0));
    
    return result;
}

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

v4
operator*(m4x4 M, v4 V)
{
    v4 Result = {
        M.E[0][0] * V.x + M.E[0][1] * V.y + M.E[0][2] * V.z + M.E[0][3] * V.w,
        M.E[1][0] * V.x + M.E[1][1] * V.y + M.E[1][2] * V.z + M.E[1][3] * V.w,
        M.E[2][0] * V.x + M.E[2][1] * V.y + M.E[2][2] * V.z + M.E[2][3] * V.w,
        M.E[3][0] * V.x + M.E[3][1] * V.y + M.E[3][2] * V.z + M.E[3][3] * V.w,
    };
    
    return Result;
}

m4x4 operator*(m4x4 A, m4x4 B)
{
    // NOTE: This is not a fast implementation
    m4x4 Result  = {};
    for (i32 r = 0; r <= 3; ++r)
    {
        for (i32 c = 0; c <= 3; ++c)
        {
            for (i32 i = 0; i <= 3; ++i)
            {
                Result.E[r][c] += A.E[r][i] * B.E[i][c];
            }
        }
    }
    
    return Result;
}

m4x4 Identity()
{
    m4x4 Result = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    
    return Result;
};

m4x4 RotateZ(f32 Angle)
{
    f32 c = cos(Angle);
    f32 s = sin(Angle);
    m4x4 Result = {
        c,    -s,   0.0f, 0.0f,
        s,    c,    0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    
    return Result;
}

m4x4 RotateX(f32 Angle)
{
    f32 c = cos(Angle);
    f32 s = sin(Angle);
    m4x4 Result = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, c,    -s,   0.0f,
        0.0f, s,    c,    0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    
    return Result;
}

m4x4 RotateY(f32 Angle)
{
    f32 c = cos(Angle);
    f32 s = sin(Angle);
    m4x4 Result = {
        c,    0.0f, s,    0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        -s,   0.0f, c,    0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    
    return Result;
}

m4x4 Scale(f32 S)
{
    m4x4 Result = {
        S, 0.0f, 0.0f, 0.0f,
        0.0f, S, 0.0f, 0.0f,
        0.0f, 0.0f, S, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    
    return Result;
}

m4x4 Translation(v3 T)
{
    m4x4 Result = {
        1.0f, 0.0f, 0.0f, T.x,
        0.0f, 1.0f, 0.0f, T.y,
        0.0f, 0.0f, 1.0f, T.z,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    
    return Result;
}


m4x4 GetViewMatrix(v3 CameraP, v3 TargetP, v3 WorldUp)
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

m4x4 LookAtRH(v3 Eye, v3 Centre, v3 Up)
{
    // In glm matricies are stored in column major format, e.g.
    // struct mat4 { vec4<float> columns[4] };
    
    // Gets the sameresult as my look at function, just creates the camera basis vectors
    // differently.
    
    v3 f = Normalize(Centre - Eye);
    v3 s = Normalize(Cross(f, Up));
    v3 u = Cross(s, f);
    
    m4x4 Result = Identity();
    
    // Copied from GLM but switched row and column indices
    Result.E[0][0] = s.x;
    Result.E[0][1] = s.y;
    Result.E[0][2] = s.z;
    Result.E[1][0] = u.x;
    Result.E[1][1] = u.y;
    Result.E[1][2] = u.z;
    Result.E[2][0] =-f.x;
    Result.E[2][1] =-f.y;
    Result.E[2][2] =-f.z;
    Result.E[0][3] =-Inner(s, Eye);
    Result.E[1][3] =-Inner(u, Eye);
    Result.E[2][3] = Inner(f, Eye); // no negation here
    
    return Result;
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

m4x4 OrthographicProjection(f32 Left, f32 Right, f32 Bottom, f32 Top, f32 Near, f32 Far)
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

m4x4 OrthographicProjection(f32 Left, f32 Right, f32 Bottom, f32 Top)
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

constexpr f32 ToRadians(f32 Degrees)
{
    return Degrees * 0.01745329251994329576923690768489;
}

#endif //DRAW_3D_MATH_H
