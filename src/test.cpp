
#include "os.h"
#include "base.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>

// Returns non-null terminated string
string UTF8FromUTF16(arena *Arena, wchar_t *String16, u32 String16Length);
// Returns null-terminated string
wchar_t *UTF16FromUTF8(arena *Arena, u8 *String8, int String8Length);

#if 0
void PageProtection(char *Buffer, DWORD ProtectionFlags)
{
    if (ProtectionFlags & PAGE_EXECUTE)            strcat_s(Buffer, 4096, "PAGE_EXECUTE|");
    if (ProtectionFlags & PAGE_EXECUTE_READ)       strcat_s(Buffer, 4096, "PAGE_EXECUTE_READ|");
    if (ProtectionFlags & PAGE_EXECUTE_READWRITE)  strcat_s(Buffer, 4096, "PAGE_EXECUTE_READWRITE|");
    if (ProtectionFlags & PAGE_EXECUTE_WRITECOPY)  strcat_s(Buffer, 4096, "PAGE_EXECUTE_WRITECOPY|");
    if (ProtectionFlags & PAGE_NOACCESS)           strcat_s(Buffer, 4096, "PAGE_NOACCESS|");
    if (ProtectionFlags & PAGE_READONLY)           strcat_s(Buffer, 4096, "PAGE_READONLY|");
    if (ProtectionFlags & PAGE_READWRITE)          strcat_s(Buffer, 4096, "PAGE_READWRITE|");
    if (ProtectionFlags & PAGE_WRITECOPY)          strcat_s(Buffer, 4096, "PAGE_WRITECOPY|");
    if (ProtectionFlags & PAGE_TARGETS_INVALID)    strcat_s(Buffer, 4096, "PAGE_TARGETS_INVALID|");
    
    if (ProtectionFlags & PAGE_GUARD)              strcat_s(Buffer, 4096, "PAGE_GUARD|");
    if (ProtectionFlags & PAGE_NOCACHE)            strcat_s(Buffer, 4096, "PAGE_NOCACHE|");
    if (ProtectionFlags & PAGE_WRITECOMBINE)       strcat_s(Buffer, 4096, "PAGE_WRITECOMBINE|");
}

void QueryPages(arena *Arena)
{
    u64 PageIndex = 0;
    for (u64 i = 0; i < Arena->Commit + 4096; i += 4096)
    {
        MEMORY_BASIC_INFORMATION Info = {};
        SIZE_T Size = VirtualQuery(Arena->Base + i, &Info, sizeof(Info));
        Assert(Size);
        
        const char *State = "UNKNOWN";
        if (Info.State == MEM_COMMIT) State = "MEM_COMMIT";
        if (Info.State == MEM_FREE) State = "MEM_FREE";
        if (Info.State == MEM_RESERVE) State = "MEM_RESERVE";
        
        char InitialProtection[4069] = {};
        PageProtection(InitialProtection, Info.AllocationProtect); 
        
        char Protection[4069] = {};
        PageProtection(Protection, Info.Protect); 
        
        printf("%llu:  %p region size: %llu, state %s, protect: %s\n", 
               PageIndex,
               Info.BaseAddress, 
               Info.RegionSize, State, 
               Protection);
        
        PageIndex += 1;
    }
    
    printf("\n");
}
#endif

// func0 0
// a
// fun1 a
// b
// fun2 b
// a

void NestedScratchReuseSame(u32 Depth)
{
    if (Depth == 8)
    {
        return;
    }
    
    temp_arena Scratch = GetScratch();
    u32 *Value = PushArray(Scratch.Arena, 10, u32);
    for (u32 i = 0; i < 10; ++i) 
    {
        Value[i] = i + (Depth * 10);
    }
    
    NestedScratchReuseSame(Depth + 1);
    
    for (u32 i = 0; i < 10; ++i) 
    {
        Assert(Value[i] == i + (Depth * 10));
    }
    
    ReleaseScratch(Scratch);
    return;
}

void NestedScratchReuseAlternating(u32 Depth, arena *Arena)
{
    if (Depth == 8)
    {
        return;
    }
    
    temp_arena Scratch = GetScratch(Arena);
    u32 *Value = PushArray(Scratch.Arena, 10, u32);
    for (u32 i = 0; i < 10; ++i) 
    {
        Value[i] = i + (Depth * 10);
    }
    
    NestedScratchReuseAlternating(Depth + 1, Scratch.Arena);
    
    for (u32 i = 0; i < 10; ++i) 
    {
        Assert(Value[i] == i + (Depth * 10));
    }
    
    ReleaseScratch(Scratch);
}

void NestedScratchReuseAlternatingWithExtra(u32 Depth, arena *Arena1, arena *Arena2)
{
    if (Depth == 8)
    {
        return;
    }
    
    bool ReleaseArena2 = false;
    if (Arena2 == 0)
    {
        Arena2 = CreateArena(GB(1));
        ReleaseArena2 = true;
    }
    
    temp_arena Scratch = GetScratch(Arena1, Arena2);
    u32 *Value = PushArray(Scratch.Arena, 10, u32);
    for (u32 i = 0; i < 10; ++i) 
    {
        Value[i] = i + (Depth * 10);
    }
    
    NestedScratchReuseAlternatingWithExtra(Depth + 1, Scratch.Arena, Arena2);
    
    for (u32 i = 0; i < 10; ++i) 
    {
        Assert(Value[i] == i + (Depth * 10));
    }
    
    ReleaseScratch(Scratch);
    
    if (ReleaseArena2)
    {
        FreeArena(Arena2);
    }
}

void RunTests()
{
    printf("Running tests...\n");
    
    arena *TestDataArena = CreateArena(GB(1));
    
    string ExePath = OS_GetExecutablePath(TestDataArena);
    string NMinus2Dirs = DirectoryFromPath(StringChop(DirectoryFromPath(ExePath), 1));
    string TestDir = PushStringf(TestDataArena, "%.*s%s", (int)NMinus2Dirs.Length, NMinus2Dirs.Str, "test/");
    
#if 0
    {
        arena *Arena = CreateArena(GB(1));
        QueryPages(Arena);
        u32 Count = 10000;
        u32 *Array1 = PushArray(Arena, Count, u32);
        QueryPages(Arena);
        for (u32 i = 0; i < Count; ++i)
        {
            Array1[i] = i;
        }
        
        u32 Load = Array1[Count];
        Array1[Count] = 255;
        
        int x = 99;
        FreeArena(Arena);
    }
#endif
    
    struct test
    {
        u32 A;
        u32 B;
        u32 C;
        u64 D;
    };
    
#if 0
    {
        // Test crashes after using past end of allocation
        arena *Arena = CreateArena(GB(1));
        test *Test = PushArray(Arena, 2, test);
        Test->A = 0;
        Test->B = 1;
        Test->D = 3;
        Test->C = 2;
        
        Test = Test + 1;
        Test->A = 0;
        Test->B = 1;
        Test->D = 3;
        Test->C = 2;
        
        test *After = Test + 1;
        After->B = 2;
        u32 Load = After->A;
        
        int x = 99;
        FreeArena(Arena);
    }
#endif
    
#if 0
    {
        arena *Arena = CreateArena(GB(1));
        
        QueryPages(Arena);
        u32 *Array1 = PushArray(Arena, 100, u32);
        for (u32 i = 0; i < 100; ++i)
        {
            Array1[i] = i;
        }
        
        QueryPages(Arena);
        u32 *Array2 = PushArray(Arena, 1000, u32);
        for (u32 i = 0; i < 1000; ++i)
        {
            Array2[i] = i;
        }
        
        QueryPages(Arena);
        u32 *Array3 = PushArray(Arena, 10000, u32);
        for (u32 i = 0; i < 10000; ++i)
        {
            Array3[i] = i;
        }
        
        QueryPages(Arena);
        PopSize(Arena, 40000);
        
        QueryPages(Arena);
        u32 *Array4 = PushArray(Arena, 10000, u32);
        QueryPages(Arena);
        for (u32 i = 0; i < 10000; ++i)
        {
            Array4[i] = i;
        }
        
        QueryPages(Arena);
        u64 *Array5 = PushArray(Arena, 100000, u64);
        for (u32 i = 0; i < 10000; ++i)
        {
            Array5[i] = i;
        }
        
        QueryPages(Arena);
        PopToPosition(Arena, MB(900)); // SHould do nothing
        
        QueryPages(Arena);
        PopSize(Arena, 500);
        
        QueryPages(Arena);
        
        ClearArena(Arena);
        QueryPages(Arena);
        u64 *Array6 = PushArray(Arena, 234223, u64);
        for (u64 i = 0; i < 234223; ++i)
        {
            Array6[i] = i;
        }
        
        QueryPages(Arena);
        
        // Non multiple of 4 size
        u8 *Array7 = (u8 *)PushSize_(Arena, 74, 4);
        for (u32 i = 0; i < 74; ++i)
        {
            Array7[i] = 3;
        }
        
        // Because of alignment and size of this allocation there is a small gap between the end of the allocation
        // and the start of the guard page
        Array7[74] = 4;
        u8 Temp = Array7[74];
        Array7[75] = Temp;
        
        
        // Small size
        u8 *Array8 = (u8 *)PushSize_(Arena, 3, 4);
        for (u32 i = 0; i < 3; ++i)
        {
            Array8[i] = 2;
        }
        
        Array8[3] = 1;
        
        FreeArena(Arena);
    }
#endif
    
    {
        printf("NUM_ARGS(); = %d\n", NUM_ARGS());
        printf("NUM_ARGS(A); = %d\n", NUM_ARGS(A));
        printf("NUM_ARGS(A,B); = %d\n", NUM_ARGS(A,B));
        printf("NUM_ARGS(A,B,C); = %d\n", NUM_ARGS(A,B,C));
        printf("NUM_ARGS(A, B, C); = %d\n", NUM_ARGS(A, B, C));
        printf("NUM_ARGS(A , B , C ); = %d\n", NUM_ARGS(A , B , C ));
    }
    
    {
        arena *Arena = CreateArena(GB(300));
        
        u32 *Array1 = PushArray(Arena, MB(30) / 4, u32);
        u32 *Array2 = PushArray(Arena, MB(30) / 4, u32);
        
        for (u32 i = 0; i < 10000; ++i)
        {
            Array1[i] = i;
            Array2[i] = i * 2;
        }
        
        MemoryZero(sizeof(u32) * 10000, Array1);
        MemoryCopy(sizeof(u32) * 10000, Array1, Array2);
        MemoryCopyOverlapped(sizeof(u32) * 10000, Array1, Array2);
        
        u64 Pos = Arena->Pos;
        
        f32 *F32Array1 = (f32 *)PushSize_(Arena, MB(40), 4, ArenaPushFlags_None);
        f32 *F32Array2 = (f32 *)PushCopy_(Arena, MB(40), F32Array1, alignof(f32));
        MemoryZero(MB(40), F32Array1);
        MemoryZero(MB(40), F32Array2);
        
        PopToPosition(Arena, Pos);
        
        u8 *Array3 = PushArray(Arena, MB(100), u8);
        
        for (u32 i = 0; i < MB(100); ++i)
            Array3[i] = (u8)i;
        
        temp_arena TempMemory1 = BeginTempArena(Arena);
        u8 *Array4 = PushArray(Arena, 100, u8);
        MemoryZero(100, Array4);
        temp_arena TempMemory2 = BeginTempArena(Arena);
        u8 *Array5 = PushArray(Arena, 100, u8);
        MemoryZero(100, Array5);
        
        EndTempArena(TempMemory2);
        EndTempArena(TempMemory1); 
        
        temp_arena Scratch = GetScratch();
        
        u16 *Array6 = PushArray(Scratch.Arena, 10000, u16);
        MemoryZero(10000 * 2, Array6);
        arena *Struct = PushStruct(Scratch.Arena, arena);
        Struct->Base = 0;
        Struct->TempCount = 239487239;
        
        ReleaseScratch(Scratch);
        
        // Should this be allowed?
        //u32 *ZeroByteAlloc = PushArray(Arena, 0, u32);
        //Assert(ZeroByteAlloc != 0);
        
        ClearArena(Arena);
        
        Assert(PushArray(Arena, MB(100), u8));
        
        FreeArena(Arena);
    }
    
    {
        // Test zero size allocations
        temp_arena Scratch = GetScratch();
        u32 *ZeroArray = PushArray(Scratch.Arena, 0, u32);
        Assert(ZeroArray == 0);
        string S = PushString(Scratch.Arena, Strlit(""));
        Assert(S.Length == 0);
        ReleaseScratch(Scratch);
    }
    
    {
        NestedScratchReuseSame(0);
        NestedScratchReuseAlternating(0, 0);
        NestedScratchReuseAlternatingWithExtra(0, 0, 0);
    }
    
    {
        temp_arena Scratch = GetScratch();
        string String1 = StringConcat(Scratch.Arena, Strlit("ABC"), Strlit("DEF"), Strlit(""), Strlit("GH"));
        Assert(StringsAreEqual(String1, Strlit("ABCDEFGH")));
        
        string String2 = StringConcat(Scratch.Arena, Strlit("ABC"));
        Assert(StringsAreEqual(String2, Strlit("ABC")));
        
        string String3 = StringConcat(Scratch.Arena, Strlit(""));
        Assert(StringsAreEqual(String3, Strlit("")));
        
        string String4 = StringConcat(Scratch.Arena);
        Assert(StringsAreEqual(String4, Strlit("")));
        
        ReleaseScratch(Scratch);
    }
    
    {
        arena *Arena = CreateArena(GB(1));
        
        const char *conststr = "0123456789";
        string NumberString = PushStringf(Arena, (char *)"%s %u ABCDEF, !%f", conststr, 2342u, 0.0002342234f);
        
        string StringUpper = Strlit("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        string StringLower = Strlit("abcdefghijklmnopqrstuvwxyz");
        string StringNumbers = Strlit("0123456789");
        
        u8 C1 = StringGetChar(StringUpper, 25);
        Assert(C1 == 'Z');
        u8 C2 = StringGetChar(StringUpper, 26);
        Assert(C2 == 0);
        
        for (u32 i = 0; i < 26; ++i)
        {
            Assert(IsUpper(StringUpper[i]));
            Assert(IsLower(StringLower[i]));
            Assert(IsAlpha(StringUpper[i]) && IsAlpha(StringLower[i]));
        }
        
        for (u32 i = 0; i < 10; ++i)
        {
            Assert(IsNumber(StringNumbers[i]));
        }
        
        u64 Index1 = StringFindChar(StringUpper, 'Y', 10);
        Assert(Index1 == 24);
        
        u64 Index2 = StringFindChar(StringUpper, '7');
        Assert(Index2 == StringUpper.Length);
        
        u64 Index3 = StringFindLastChar(StringUpper, 'D');
        Assert(Index3 == 3);
        
        u64 Index4 = StringFindLastChar(Strlit(""), '6');
        Assert(Index4 == 0);
        
        
        string Haystack1 = Strlit("THE CAT end");
        u64 Pos1 = StringFindStr(Haystack1, Strlit("CAT"));
        Assert(Pos1 == 4);
        
        u64 Pos2 = StringFindStr(Haystack1, Strlit("CATE"));
        Assert(Pos2 == Haystack1.Length);
        
        u64 Pos3 = StringFindStr(Haystack1, Strlit(""));
        Assert(Pos3 == Haystack1.Length);
        
        u64 Pos4 = StringFindStr(Strlit(""), Strlit(""));
        Assert(Pos4 == 0);
        
        u64 Pos5 = StringFindStr(Strlit(""), Strlit("ADE"));
        Assert(Pos5 == 0);
        
        string Haystack2 = Strlit("LE CHAT");
        
        u64 Pos6 = StringFindStr(Haystack2, Strlit("CHATE"));
        Assert(Pos6 == Haystack2.Length);
        
        u64 Pos7 = StringFindStr(Haystack2, Strlit("LE CHATE"));
        Assert(Pos7 == Haystack2.Length);
        
        u64 Pos8 = StringFindStr(Haystack2, Strlit("LE CHAT"));
        Assert(Pos8 == 0);
        
        u64 Pos9 = StringFindStr(Haystack2, Strlit("T"));
        Assert(Pos9 == 6);
        
        string Haystack3 = Strlit("CA cat CATeCATCATE ");
        u64 Pos10 = StringFindStr(Haystack3, Strlit("CATE"));
        Assert(Pos10 == 14);
        
        
        Assert(StringContainsChar(Strlit("ABC"), 'B'));
        Assert(StringContainsChar(Strlit("B"), 'B'));
        Assert(!StringContainsChar(Strlit(""), 'B'));
        
        Assert(StringContainsStr(Strlit("abcdefghij"), Strlit("defg")));
        Assert(!StringContainsStr(Strlit("abc"), Strlit("abcd")));
        Assert(!StringContainsStr(Strlit(""), Strlit("abcd")));
        Assert(!StringContainsStr(Strlit("abc"), Strlit("")));
        
        //                                               01234567890123456789
        u64 SlashIndex1 = StringFindLastSlash(Strlit("C:/dev/test/file.exe"));
        Assert(SlashIndex1 == 11);
        
        string FileExe = Strlit("file.exe");
        u64 SlashIndex2 = StringFindLastSlash(FileExe);
        Assert(SlashIndex2 == FileExe.Length);
        
        string TestDotTarGz = Strlit("test.tar.gz");
        string TestDotObjDot = Strlit("test.obj.");
        string TestDotDotObj = Strlit("test..obj");
        string Test = Strlit("test");
        string Dot = Strlit(".");
        RemoveExtension(&FileExe);
        RemoveExtension(&TestDotTarGz);
        RemoveExtension(&TestDotObjDot);
        RemoveExtension(&TestDotDotObj);
        RemoveExtension(&Test);
        RemoveExtension(&Dot);
        
        Assert(StringsAreEqual(FileExe, Strlit("file"))); 
        Assert(StringsAreEqual(TestDotTarGz, Strlit("test.tar"))); 
        Assert(StringsAreEqual(TestDotObjDot, Strlit("test.obj"))); 
        Assert(StringsAreEqual(TestDotDotObj, Strlit("test."))); 
        Assert(StringsAreEqual(Test, Strlit("test"))); 
        Assert(StringsAreEqual(Dot, Strlit(""))); 
        
        Assert(StringsAreEqual(Strlit("file.exe"), FilenameFromPath(Strlit("C:/dev/test/file.exe")))); 
        Assert(StringsAreEqual(Strlit("file.exe"), FilenameFromPath(Strlit("C://dev/test////file.exe")))); 
        Assert(StringsAreEqual(Strlit("file.exe"), FilenameFromPath(Strlit("/c/dev/test/file.exe")))); 
        Assert(StringsAreEqual(Strlit("file"), FilenameFromPath(Strlit("/c/dev/test/file")))); 
        Assert(StringsAreEqual(Strlit("file"), FilenameFromPath(Strlit("/c/.dev/test //file")))); 
        Assert(StringsAreEqual(Strlit(""), FilenameFromPath(Strlit("/c/dev/test/file/")))); 
        Assert(StringsAreEqual(Strlit(""), FilenameFromPath(Strlit("file")))); 
        Assert(StringsAreEqual(Strlit(""), FilenameFromPath(Strlit("/")))); 
        Assert(StringsAreEqual(Strlit("c"), FilenameFromPath(Strlit("/c")))); 
        Assert(StringsAreEqual(Strlit("elmo.doc"), FilenameFromPath(Strlit("C:/abc\\def//hjkl\\\\sdfsdf/elmo.doc")))); 
        
        Assert(StringsAreEqual(Strlit("C:/dev/test/"), DirectoryFromPath(Strlit("C:/dev/test/file.exe")))); 
        Assert(StringsAreEqual(Strlit("C://dev/test////"), DirectoryFromPath(Strlit("C://dev/test////file.exe")))); 
        Assert(StringsAreEqual(Strlit("/c/dev/test/"), DirectoryFromPath(Strlit("/c/dev/test/file.exe")))); 
        Assert(StringsAreEqual(Strlit("/c/dev/test/"), DirectoryFromPath(Strlit("/c/dev/test/file")))); 
        Assert(StringsAreEqual(Strlit("/c/.dev/test //"), DirectoryFromPath(Strlit("/c/.dev/test //file")))); 
        Assert(StringsAreEqual(Strlit("/c/dev/test/file/"), DirectoryFromPath(Strlit("/c/dev/test/file/")))); 
        Assert(StringsAreEqual(Strlit(""), DirectoryFromPath(Strlit("file")))); 
        Assert(StringsAreEqual(Strlit("/"), DirectoryFromPath(Strlit("/")))); 
        Assert(StringsAreEqual(Strlit("/"), DirectoryFromPath(Strlit("/c")))); 
        Assert(StringsAreEqual(Strlit("C:/abc\\def//hjkl\\\\sdfsdf/"), DirectoryFromPath(Strlit("C:/abc\\def//hjkl\\\\sdfsdf/elmo.doc")))); 
        
        temp_arena Scratch = GetScratch();
        
        string_list List = {};
        StringListAppend(Scratch.Arena, &List, (char *)"Hello");
        StringListAppend(Scratch.Arena, &List, Strlit(" World I "));
        StringListAppend(Scratch.Arena, &List, Strlit(""));
        StringListAppend(Scratch.Arena, &List, Strlit("Am "));
        StringListAppend(Scratch.Arena, &List, Strlit("Here."));
        
        string WholeString = StringListJoin(Scratch.Arena, &List);
        Assert(StringsAreEqual(WholeString, Strlit("Hello World I Am Here.")));
        
        string_list List2 = {};
        Assert(StringsAreEqual(StringListJoin(Scratch.Arena, &List2), Strlit("")));
        
        ReleaseScratch(Scratch);
        
        
        
        Assert(StringsAreEqual(StringPrefix(Strlit("ABCDEF"), 3), Strlit("ABC")));
        Assert(StringsAreEqual(StringPrefix(Strlit("ABCDEF"), 6), Strlit("ABCDEF")));
        Assert(StringsAreEqual(StringPrefix(Strlit("ABCDEF"), 10), Strlit("ABCDEF")));
        Assert(StringsAreEqual(StringPrefix(Strlit("ABCDEF"), 0), Strlit("")));
        Assert(StringsAreEqual(StringPrefix(Strlit(""), 1), Strlit("")));
        
        Assert(StringsAreEqual(StringSuffix(Strlit("ABCDEF"), 3), Strlit("DEF")));
        Assert(StringsAreEqual(StringSuffix(Strlit("ABCDEF"), 5), Strlit("BCDEF")));
        Assert(StringsAreEqual(StringSuffix(Strlit("ABCDEF"), 10), Strlit("ABCDEF")));
        Assert(StringsAreEqual(StringSuffix(Strlit("ABCDEF"), 0), Strlit("")));
        Assert(StringsAreEqual(StringSuffix(Strlit(""), 1), Strlit("")));
        
        Assert(StringsAreEqual(SubstrRange(Strlit("ABCDEF"), 0, 6), Strlit("ABCDEF")));
        Assert(StringsAreEqual(SubstrRange(Strlit("ABCDEF"), 1, 3), Strlit("BC")));
        Assert(StringsAreEqual(SubstrRange(Strlit("ABCDEF"), 3, 3), Strlit("")));
        Assert(StringsAreEqual(SubstrRange(Strlit("ABCDEF"), 3, 4), Strlit("D")));
        Assert(StringsAreEqual(SubstrRange(Strlit("ABCDEF"), 3, 10), Strlit("DEF")));
        Assert(StringsAreEqual(SubstrRange(Strlit("ABCDEF"), 10, 10), Strlit("")));
        Assert(StringsAreEqual(SubstrRange(Strlit(""), 0, 1), Strlit("")));
        
        Assert(StringsAreEqual(Substr(Strlit("ABCDEF"), 4, 1), Strlit("E")));
        Assert(StringsAreEqual(Substr(Strlit("ABCDEF"), 2, 3), Strlit("CDE")));
        Assert(StringsAreEqual(Substr(Strlit("ABCDEF"), 5, 10), Strlit("F")));
        Assert(StringsAreEqual(Substr(Strlit("ABCDEF"), 6, 1), Strlit("")));
        Assert(StringsAreEqual(Substr(Strlit("ABCDEF"), 1, 0), Strlit("")));
        Assert(StringsAreEqual(Substr(Strlit(""), 0, 1), Strlit("")));
        
        FreeArena(Arena);
    }
    
    {
        
        parsed_num Result = StringParseNum(Strlit("123"));
        Assert(Result.Value == 123);
        
        Result = StringParseNum(Strlit("0"));
        Assert(Result.Value == 0);
        
        Result = StringParseNum(Strlit("000123"));
        Assert(Result.Value == 123);
        
        Result = StringParseNum(Strlit("00000000"));
        Assert(Result.Value == 0);
        
        Result = StringParseNum(Strlit("23482736278238"));
        Assert(Result.Value == 23482736278238);
        
        Result = StringParseNum(Strlit("18446744073709551615"));
        Assert(Result.Value == 18446744073709551615LLU);
        
        Result = StringParseNum(Strlit("018446744073709551615"));
        Assert(Result.Value == 18446744073709551615LLU);
        
        Result = StringParseNum(Strlit("18446744073709551616"));
        Assert(Result.Error);
        
        Result = StringParseNum(Strlit("-1"));
        Assert(Result.Error);
        
        Result = StringParseNum(Strlit(""));
        Assert(Result.Error);
        
        Result = StringParseNum(Strlit("acd"));
        Assert(Result.Error);
        
        Result = StringParseNum(Strlit("12acd"));
        Assert(Result.Error);
        
        Result = StringParseNum(Strlit("acd123"));
        Assert(Result.Error);
        
        Result = StringParseNum(Strlit(" 12"));
        Assert(Result.Error);
        
        Result = StringParseNum(Strlit(" "));
        Assert(Result.Error);
        
        Result = StringParseNum(Strlit("12 "));
        Assert(Result.Error);
    }
    
    
    {
        
        //u8 Contents[] = "lsdkfj;asdlkfj;alskdjf;alskdjf;lkasdjf;l";
        
        // Longest path that I can make a file with, 268 chars including prefix
        //string LongFileName = Strlit("\\\\?\\C:\\files\\ABCDEFGHIJKLMNOPQRSTjkhsdkfjghsdklfjghlsdkjfhglksdjfhglksjdfhglksdjfhglkjdhsfgldksfjghlsdkfjhglkgjhsdlfkjghsldkfjghlsdkfjhglsdkjfhgldfhjgllkdjfhlkdasjhflsdkjfhlasjdhfljkasdhflkjahsdflhasdlkfjhasldkfhalsdkjfhlaskdjhflkasdhflaksdjhfakalaks0123456789abcdefgh");
        
        //Assert(OS_WriteEntireFile(sizeof(Contents), Contents, LongFileName));
        
        // Doesn't work without prefix
        //Assert(OS_WriteEntireFile(sizeof(Contents), Contents, Strlit("C:\\files\\ABCDEFGHIJKLMNOPQRSTjkhsdkfjghsdklfjghlsdkjfhglksdjfhglksjdfhglksdjfhglkjdhsfgldksfjghlsdkfjhglkgjhsdlfkjghsldkfjghlsdkfjhglsdkjfhgldfhjgllkdjfhlkdasjhflsdkjfhlasjdhfljkasdhflkjahsdflhasdlkfjhasldkfhalsdkjfhlaskdjhflkasdhflaksdjhfakalaks0123456789abcdefgh")));
        
        //Assert(OS_DeleteFile(LongFileName));
    }
    {
        arena *Arena = CreateArena(GB(10));
        
#if BASE_OS_WINDOWS
        string String8 = UTF8FromUTF16(Arena, (wchar_t *)L"Hellow", 6);
        wchar_t *String16 = UTF16FromUTF8(Arena, (u8 *)"Hellow", 6);
        (void)String16;
#endif
        
        string ExePath = OS_GetExecutablePath(Arena);
        Assert(ExePath.Length > 0);
        
        // TestDir is a directory not a file so shouldn't exist
        Assert(!OS_FileExists(TestDir));
        Assert(!OS_FileExists(PushStringf(Arena, "%.*s%s", (int)TestDir.Length, TestDir.Str, "NON_EXISTENT_FILE")));
        
        string File1Path = PushStringf(Arena, "%.*s%s", (int)TestDir.Length, TestDir.Str, "file1.txt");
        Assert(OS_FileExists(File1Path));
        Assert(OS_GetFileSize(File1Path) == 94);        
        string File = OS_ReadEntireFile(Arena, File1Path);
        Assert(File.Length == 94);
        Assert(File.Str);
        
#if 0
        {
            u64 TotalSize = GB(5);
            u64 ChunkSize = TotalSize / 32;
            Assert(TotalSize % 32 == 0);
            u32 WriteCount = 32;
            u8 *Chunk = PushArray(Arena, ChunkSize, u8);
            
            FILE *fp = fopen("large.big", "wb");
            for (u32 i = 0; i < WriteCount; ++i)
            {
                fwrite(Chunk, ChunkSize, 1, fp);
            }
            fclose(fp);
            
            PopSize(Arena, ChunkSize);
            
            OS__file_contents LargeFile = OS_ReadEntireFile(Arena, Strlit("large.big"));
            
            DeleteFile("large.big");
        }
#endif
        
        u64 Data[] = { 0x3234234, 0x23423423, 0x0349538450, 0x93213, 0x34882349 };
        string WriteFilePath = PushStringf(Arena, "%.*s%s", (int)TestDir.Length, TestDir.Str, "writefile.test");
        
        // If this fails it might be that the file already exists 
        Assert(OS_WriteEntireFile(sizeof(Data), (u8 *)Data, WriteFilePath));
        
        string RoundTripFile = OS_ReadEntireFile(Arena, WriteFilePath);
        Assert(RoundTripFile.Length == sizeof(Data));
        Assert(MemoryCompare(sizeof(Data), Data, RoundTripFile.Str));
        
        Assert(OS_DeleteFile(WriteFilePath));
        
        FreeArena(Arena);
    }
    
    {
        v2 A = V2(0.1f, 1.0f);
        v3 B = V3(0.1f, 1.0f, 5.234234234f);
        v4 C = V4(0.000000001f, 1231231231231231.0f, 3423242342345.234234234f, 0.0f);
        m4x4 Mat = Identity();
        m4x4 Translate = Translation(B);
        
        static_assert(S8Max == INT8_MAX, "");
        static_assert(S8Min == INT8_MIN, "");
        
        static_assert(S16Max == INT16_MAX, "");
        static_assert(S16Min == INT16_MIN, "");
        
        static_assert(S32Max == INT32_MAX, "");
        static_assert(S32Min == INT32_MIN, "");
        
        static_assert(S64Max == INT64_MAX, "");
        static_assert(S64Min == INT64_MIN, "");
        
        static_assert(U8Max == UINT8_MAX, "");
        static_assert(U16Max == UINT16_MAX, "");
        static_assert(U32Max == UINT32_MAX, "");
        static_assert(U64Max == UINT64_MAX, "");
        
        static_assert(F32Max == FLT_MAX, "");
        static_assert(F32Min == -FLT_MAX, "");
        static_assert(F64Max == DBL_MAX, "");
        static_assert(F64Min == -DBL_MAX, "");
        
        static_assert(Pi32 == (f32)M_PI, "Pi32 not equal to M_PI");
    }
    
    FreeArena(TestDataArena);
    
    printf("All tests passed\n");
}

