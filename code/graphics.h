#pragma once


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
