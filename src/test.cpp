
#include "base.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>

// Returns non-null terminated string
string UTF8FromUTF16(arena *Arena, wchar_t *String16, u32 String16Length);
// Returns null-terminated string
wchar_t *UTF16FromUTF8(arena *Arena, u8 *String8, int String8Length);

#if BASE_OS_WINDOWS
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
    
    string ExePath = PlatformGetExecutablePath(TestDataArena);
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
        MemoryCopy(0, (char *)"", ZeroArray);
        PushString(Scratch.Arena, StrLit(""));
        ReleaseScratch(Scratch);
    }
    
    {
        NestedScratchReuseSame(0);
        NestedScratchReuseAlternating(0, 0);
        NestedScratchReuseAlternatingWithExtra(0, 0, 0);
    }
    
    {
        // Address sanitizer
        
        char *Base = (char *)PlatformMemoryReserve(4096);
        PlatformMemoryCommit(Base, 4096);
        ASAN_POISON_MEMORY_REGION(Base + 3, 4096 - 3);
        Base[0] = 'x';
        Base[1] = 'x';
        Base[2] = 'x';
        Base[3] = 'x';
        Base[4] = 'x';
        Base[5] = 'x';
        Base[6] = 'x';
        Base[7] = 'x';
        Base[8] = 'x';
    }
    
    {
        arena *Arena = CreateArena(GB(1));
        
        const char *conststr = "0123456789";
        string NumberString = PushStringf(Arena, (char *)"%s %u ABCDEF, !%f", conststr, 2342u, 0.0002342234f);
        
        string StringUpper = StrLit("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        string StringLower = StrLit("abcdefghijklmnopqrstuvwxyz");
        string StringNumbers = StrLit("0123456789");
        
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
        
        u64 Index4 = StringFindLastChar(StrLit(""), '6');
        Assert(Index4 == 0);
        
        
        string Haystack1 = StrLit("THE CAT end");
        u64 Pos1 = StringFindStr(Haystack1, StrLit("CAT"));
        Assert(Pos1 == 4);
        
        u64 Pos2 = StringFindStr(Haystack1, StrLit("CATE"));
        Assert(Pos2 == Haystack1.Length);
        
        u64 Pos3 = StringFindStr(Haystack1, StrLit(""));
        Assert(Pos3 == Haystack1.Length);
        
        u64 Pos4 = StringFindStr(StrLit(""), StrLit(""));
        Assert(Pos4 == 0);
        
        u64 Pos5 = StringFindStr(StrLit(""), StrLit("ADE"));
        Assert(Pos5 == 0);
        
        string Haystack2 = StrLit("LE CHAT");
        
        u64 Pos6 = StringFindStr(Haystack2, StrLit("CHATE"));
        Assert(Pos6 == Haystack2.Length);
        
        u64 Pos7 = StringFindStr(Haystack2, StrLit("LE CHATE"));
        Assert(Pos7 == Haystack2.Length);
        
        u64 Pos8 = StringFindStr(Haystack2, StrLit("LE CHAT"));
        Assert(Pos8 == 0);
        
        u64 Pos9 = StringFindStr(Haystack2, StrLit("T"));
        Assert(Pos9 == 6);
        
        string Haystack3 = StrLit("CA cat CATeCATCATE ");
        u64 Pos10 = StringFindStr(Haystack3, StrLit("CATE"));
        Assert(Pos10 == 14);
        
        
        Assert(StringContainsChar(StrLit("ABC"), 'B'));
        Assert(StringContainsChar(StrLit("B"), 'B'));
        Assert(!StringContainsChar(StrLit(""), 'B'));
        
        Assert(StringContainsStr(StrLit("abcdefghij"), StrLit("defg")));
        Assert(!StringContainsStr(StrLit("abc"), StrLit("abcd")));
        Assert(!StringContainsStr(StrLit(""), StrLit("abcd")));
        Assert(!StringContainsStr(StrLit("abc"), StrLit("")));
        
        //                                               01234567890123456789
        u64 SlashIndex1 = StringFindLastSlash(StrLit("C:/dev/test/file.exe"));
        Assert(SlashIndex1 == 11);
        
        string FileExe = StrLit("file.exe");
        u64 SlashIndex2 = StringFindLastSlash(FileExe);
        Assert(SlashIndex2 == FileExe.Length);
        
        string TestDotTarGz = StrLit("test.tar.gz");
        string TestDotObjDot = StrLit("test.obj.");
        string TestDotDotObj = StrLit("test..obj");
        string Test = StrLit("test");
        string Dot = StrLit(".");
        RemoveExtension(&FileExe);
        RemoveExtension(&TestDotTarGz);
        RemoveExtension(&TestDotObjDot);
        RemoveExtension(&TestDotDotObj);
        RemoveExtension(&Test);
        RemoveExtension(&Dot);
        
        Assert(StringsAreEqual(FileExe, StrLit("file"))); 
        Assert(StringsAreEqual(TestDotTarGz, StrLit("test.tar"))); 
        Assert(StringsAreEqual(TestDotObjDot, StrLit("test.obj"))); 
        Assert(StringsAreEqual(TestDotDotObj, StrLit("test."))); 
        Assert(StringsAreEqual(Test, StrLit("test"))); 
        Assert(StringsAreEqual(Dot, StrLit(""))); 
        
        Assert(StringsAreEqual(StrLit("file.exe"), FilenameFromPath(StrLit("C:/dev/test/file.exe")))); 
        Assert(StringsAreEqual(StrLit("file.exe"), FilenameFromPath(StrLit("C://dev/test////file.exe")))); 
        Assert(StringsAreEqual(StrLit("file.exe"), FilenameFromPath(StrLit("/c/dev/test/file.exe")))); 
        Assert(StringsAreEqual(StrLit("file"), FilenameFromPath(StrLit("/c/dev/test/file")))); 
        Assert(StringsAreEqual(StrLit("file"), FilenameFromPath(StrLit("/c/.dev/test //file")))); 
        Assert(StringsAreEqual(StrLit(""), FilenameFromPath(StrLit("/c/dev/test/file/")))); 
        Assert(StringsAreEqual(StrLit(""), FilenameFromPath(StrLit("file")))); 
        Assert(StringsAreEqual(StrLit(""), FilenameFromPath(StrLit("/")))); 
        Assert(StringsAreEqual(StrLit("c"), FilenameFromPath(StrLit("/c")))); 
        Assert(StringsAreEqual(StrLit("elmo.doc"), FilenameFromPath(StrLit("C:/abc\\def//hjkl\\\\sdfsdf/elmo.doc")))); 
        
        Assert(StringsAreEqual(StrLit("C:/dev/test/"), DirectoryFromPath(StrLit("C:/dev/test/file.exe")))); 
        Assert(StringsAreEqual(StrLit("C://dev/test////"), DirectoryFromPath(StrLit("C://dev/test////file.exe")))); 
        Assert(StringsAreEqual(StrLit("/c/dev/test/"), DirectoryFromPath(StrLit("/c/dev/test/file.exe")))); 
        Assert(StringsAreEqual(StrLit("/c/dev/test/"), DirectoryFromPath(StrLit("/c/dev/test/file")))); 
        Assert(StringsAreEqual(StrLit("/c/.dev/test //"), DirectoryFromPath(StrLit("/c/.dev/test //file")))); 
        Assert(StringsAreEqual(StrLit("/c/dev/test/file/"), DirectoryFromPath(StrLit("/c/dev/test/file/")))); 
        Assert(StringsAreEqual(StrLit(""), DirectoryFromPath(StrLit("file")))); 
        Assert(StringsAreEqual(StrLit("/"), DirectoryFromPath(StrLit("/")))); 
        Assert(StringsAreEqual(StrLit("/"), DirectoryFromPath(StrLit("/c")))); 
        Assert(StringsAreEqual(StrLit("C:/abc\\def//hjkl\\\\sdfsdf/"), DirectoryFromPath(StrLit("C:/abc\\def//hjkl\\\\sdfsdf/elmo.doc")))); 
        
        temp_arena Scratch = GetScratch();
        
        string_list List = {};
        StringListAppend(Scratch.Arena, &List, (char *)"Hello");
        StringListAppend(Scratch.Arena, &List, StrLit(" World I "));
        StringListAppend(Scratch.Arena, &List, StrLit(""));
        StringListAppend(Scratch.Arena, &List, StrLit("Am "));
        StringListAppend(Scratch.Arena, &List, StrLit("Here."));
        
        string WholeString = StringListJoin(Scratch.Arena, &List);
        Assert(StringsAreEqual(WholeString, StrLit("Hello World I Am Here.")));
        
        string_list List2 = {};
        Assert(StringsAreEqual(StringListJoin(Scratch.Arena, &List2), StrLit("")));
        
        ReleaseScratch(Scratch);
        
        
        
        Assert(StringsAreEqual(StringPrefix(StrLit("ABCDEF"), 3), StrLit("ABC")));
        Assert(StringsAreEqual(StringPrefix(StrLit("ABCDEF"), 6), StrLit("ABCDEF")));
        Assert(StringsAreEqual(StringPrefix(StrLit("ABCDEF"), 10), StrLit("ABCDEF")));
        Assert(StringsAreEqual(StringPrefix(StrLit("ABCDEF"), 0), StrLit("")));
        Assert(StringsAreEqual(StringPrefix(StrLit(""), 1), StrLit("")));
        
        Assert(StringsAreEqual(StringSuffix(StrLit("ABCDEF"), 3), StrLit("DEF")));
        Assert(StringsAreEqual(StringSuffix(StrLit("ABCDEF"), 5), StrLit("BCDEF")));
        Assert(StringsAreEqual(StringSuffix(StrLit("ABCDEF"), 10), StrLit("ABCDEF")));
        Assert(StringsAreEqual(StringSuffix(StrLit("ABCDEF"), 0), StrLit("")));
        Assert(StringsAreEqual(StringSuffix(StrLit(""), 1), StrLit("")));
        
        Assert(StringsAreEqual(SubstrRange(StrLit("ABCDEF"), 0, 6), StrLit("ABCDEF")));
        Assert(StringsAreEqual(SubstrRange(StrLit("ABCDEF"), 1, 3), StrLit("BC")));
        Assert(StringsAreEqual(SubstrRange(StrLit("ABCDEF"), 3, 3), StrLit("")));
        Assert(StringsAreEqual(SubstrRange(StrLit("ABCDEF"), 3, 4), StrLit("D")));
        Assert(StringsAreEqual(SubstrRange(StrLit("ABCDEF"), 3, 10), StrLit("DEF")));
        Assert(StringsAreEqual(SubstrRange(StrLit("ABCDEF"), 10, 10), StrLit("")));
        Assert(StringsAreEqual(SubstrRange(StrLit(""), 0, 1), StrLit("")));
        
        Assert(StringsAreEqual(Substr(StrLit("ABCDEF"), 4, 1), StrLit("E")));
        Assert(StringsAreEqual(Substr(StrLit("ABCDEF"), 2, 3), StrLit("CDE")));
        Assert(StringsAreEqual(Substr(StrLit("ABCDEF"), 5, 10), StrLit("F")));
        Assert(StringsAreEqual(Substr(StrLit("ABCDEF"), 6, 1), StrLit("")));
        Assert(StringsAreEqual(Substr(StrLit("ABCDEF"), 1, 0), StrLit("")));
        Assert(StringsAreEqual(Substr(StrLit(""), 0, 1), StrLit("")));
        
        FreeArena(Arena);
    }
    
    {
        
        //u8 Contents[] = "lsdkfj;asdlkfj;alskdjf;alskdjf;lkasdjf;l";
        
        // Longest path that I can make a file with, 268 chars including prefix
        //string LongFileName = StrLit("\\\\?\\C:\\files\\ABCDEFGHIJKLMNOPQRSTjkhsdkfjghsdklfjghlsdkjfhglksdjfhglksjdfhglksdjfhglkjdhsfgldksfjghlsdkfjhglkgjhsdlfkjghsldkfjghlsdkfjhglsdkjfhgldfhjgllkdjfhlkdasjhflsdkjfhlasjdhfljkasdhflkjahsdflhasdlkfjhasldkfhalsdkjfhlaskdjhflkasdhflaksdjhfakalaks0123456789abcdefgh");
        
        //Assert(PlatformWriteEntireFile(sizeof(Contents), Contents, LongFileName));
        
        // Doesn't work without prefix
        //Assert(PlatformWriteEntireFile(sizeof(Contents), Contents, StrLit("C:\\files\\ABCDEFGHIJKLMNOPQRSTjkhsdkfjghsdklfjghlsdkjfhglksdjfhglksjdfhglksdjfhglkjdhsfgldksfjghlsdkfjhglkgjhsdlfkjghsldkfjghlsdkfjhglsdkjfhgldfhjgllkdjfhlkdasjhflsdkjfhlasjdhfljkasdhflkjahsdflhasdlkfjhasldkfhalsdkjfhlaskdjhflkasdhflaksdjhfakalaks0123456789abcdefgh")));
        
        //Assert(PlatformDeleteFile(LongFileName));
    }
    {
        arena *Arena = CreateArena(GB(10));
        
#if BASE_OS_WINDOWS
        string String8 = UTF8FromUTF16(Arena, (wchar_t *)L"Hellow", 6);
        wchar_t *String16 = UTF16FromUTF8(Arena, (u8 *)"Hellow", 6);
        (void)String16;
#endif
        
        string ExePath = PlatformGetExecutablePath(Arena);
        Assert(ExePath.Length > 0);
        
        // TestDir is a directory not a file so shouldn't exist
        Assert(!PlatformFileExists(TestDir));
        Assert(!PlatformFileExists(PushStringf(Arena, "%.*s%s", (int)TestDir.Length, TestDir.Str, "NON_EXISTENT_FILE")));
        
        string File1Path = PushStringf(Arena, "%.*s%s", (int)TestDir.Length, TestDir.Str, "file1.txt");
        Assert(PlatformFileExists(File1Path));
        Assert(PlatformGetFileSize(File1Path) == 94);        
        string File = PlatformReadEntireFile(Arena, File1Path);
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
            
            platform_file_contents LargeFile = PlatformReadEntireFile(Arena, StrLit("large.big"));
            
            DeleteFile("large.big");
        }
#endif
        
        u64 Data[] = { 0x3234234, 0x23423423, 0x0349538450, 0x93213, 0x34882349 };
        string WriteFilePath = PushStringf(Arena, "%.*s%s", (int)TestDir.Length, TestDir.Str, "writefile.test");
        
        // If this fails it might be that the file already exists 
        Assert(PlatformWriteEntireFile(sizeof(Data), (u8 *)Data, WriteFilePath));
        
        string RoundTripFile = PlatformReadEntireFile(Arena, WriteFilePath);
        Assert(RoundTripFile.Length == sizeof(Data));
        Assert(memcmp(Data, RoundTripFile.Str, sizeof(Data)) == 0);
        
        Assert(PlatformDeleteFile(WriteFilePath));
        
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
