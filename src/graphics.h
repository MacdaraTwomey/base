#pragma once

#include "base.h"

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

v3 Orbit(v3 CameraP, v3 Target, f32 AzimuthRadians, f32 InclinationRadians)
{
    // Transform to spherical coordinates, add azimuth and inclination, then transform back.
    // Assumes z-is-up right handed coordinate system.
    
    v3 P = CameraP - Target;
    
    f32 r = Length(P);
    P = NOZ(P);
    
    f32 Inclination = ACos(P.z) + InclinationRadians;
    f32 Azimuth = ATan2(P.y, P.x) + AzimuthRadians;
    
    Inclination = Clamp(Inclination, 0.01f, Pi32 - 0.01f);
    
    f32 x = Cos(Azimuth) * Sin(Inclination);
    f32 y = Sin(Azimuth) * Sin(Inclination);
    f32 z = Cos(Inclination);
    v3 RotatedP = r * V3(x, y, z);
    
    v3 Result = RotatedP + Target;
    
    return Result;
}
