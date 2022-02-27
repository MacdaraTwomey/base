#pragma once

#include "base.h"

#if 0
struct string
{
    u8 *Str;
    u64 Length;
    
    string() = default;
    string(u8 *StrData, u64 StrLength) : Str((u8 *)StrData), Length(StrLength) {}
    
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
#endif

struct c_string : public string
{
    c_string() = default;
    c_string(u8 *CStringData, u64 CStringLength) : string(CStringData, CStringLength) {}
};

// Make a new null terminated string
c_string PushStringNullTerminated(arena *Arena, string String)
{
    u64 Length = String.Length;
    u8 *StringData = (u8 *)PushSize_(Arena, Length + 1, 1, ArenaPushFlags_None);
    MemoryCopy(Length, String.Str, StringData);
    StringData[Length] = 0;
    return MakeCString(StringData, String.Length);
}

c_string CStringFromString(string String)
{
    if (String.Length > 0)
    {
        // NOTE: NULL byte counted as part of length
        Assert(String.Str[String.Length-1] == 0); // Last character is NULL byte
    } 
    
    c_string Result {String.Str, String.Length};
    return Result;
}

// Length does not include null terminator but we are guaranteed that the string is null terminated
c_string MakeCString(u8 *StringData, u64 Length)
{
    c_string Result;
    Result.Str = StringData;
    Result.Length = Length;
    return Result;
}
