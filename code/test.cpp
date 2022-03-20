

// Returns non-null terminated string
string UTF8FromUTF16(arena *Arena, wchar_t *String16, u32 String16Length);
// Returns null-terminated string
wchar_t *UTF16FromUTF8(arena *Arena, u8 *String8, int String8Length);


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

struct test
{
    u32 A;
    u32 B;
    u32 C;
    u64 D;
};


void RunTests()
{
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
    
#if 0
    {
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
        
        u32 *Array1 = PushArrayZero(Arena, MB(30) / 4, u32);
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
        
        f32 *F32Array1 = (f32 *)PushSize_(Arena, MB(40), 4, NoClear());
        f32 *F32Array2 = (f32 *)PushCopy_(Arena, MB(40), F32Array1, alignof(f32));
        MemoryZero(MB(40), F32Array1);
        MemoryZero(MB(40), F32Array2);
        
        PopToPosition(Arena, Pos);
        
        u8 *Array3 = PushArray(Arena, MB(100), u8);
        
        for (u32 i = 0; i < MB(100); ++i)
            Array3[i] = (u8)i;
        
        temp_memory TempMemory1 = BeginTempMemory(Arena);
        u8 *Array4 = PushArray(Arena, 100, u8);
        MemoryZero(100, Array4);
        temp_memory TempMemory2 = BeginTempMemory(Arena);
        u8 *Array5 = PushArray(Arena, 100, u8);
        MemoryZero(100, Array5);
        
        EndTempMemory(TempMemory1); // In wrong order
        EndTempMemory(TempMemory2);
        
        temp_memory Scratch = GetScratch();
        
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
        arena *Arena = CreateArena(GB(1));
        
        const char *conststr = "0123456789";
        string NumberString = PushFormatString(Arena, (char *)"%s %u ABCDEF, !%f", conststr, 2342u, 0.0002342234f);
        
        string StringUpper = StringLit("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        string StringLower = StringLit("abcdefghijklmnopqrstuvwxyz");
        string StringNumbers = StringLit("0123456789");
        
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
        
        u64 Index4 = StringFindLastChar(StringLit(""), '6');
        Assert(Index4 == 0);
        
        Assert(StringContainsChar(StringLit("ABC"), 'B'));
        Assert(StringContainsChar(StringLit("B"), 'B'));
        Assert(!StringContainsChar(StringLit(""), 'B'));
        
        Assert(StringContainsSubstr(StringLit("abcdefghij"), StringLit("defg")));
        Assert(!StringContainsSubstr(StringLit("abc"), StringLit("abcd")));
        Assert(!StringContainsSubstr(StringLit(""), StringLit("abcd")));
        Assert(!StringContainsSubstr(StringLit("abc"), StringLit("")));
        
        //                                               01234567890123456789
        u64 SlashIndex1 = StringFindLastSlash(StringLit("c:/dev/test/file.exe"));
        Assert(SlashIndex1 == 11);
        
        string FileExe = StringLit("file.exe");
        u64 SlashIndex2 = StringFindLastSlash(FileExe);
        Assert(SlashIndex2 == FileExe.Length);
        
        string TestDotTarGz = StringLit("test.tar.gz");
        string TestDotObjDot = StringLit("test.obj.");
        string TestDotDotObj = StringLit("test..obj");
        string Test = StringLit("test");
        string Dot = StringLit(".");
        RemoveExtension(&FileExe);
        RemoveExtension(&TestDotTarGz);
        RemoveExtension(&TestDotObjDot);
        RemoveExtension(&TestDotDotObj);
        RemoveExtension(&Test);
        RemoveExtension(&Dot);
        
        Assert(StringsAreEqual(FileExe, StringLit("file"))); 
        Assert(StringsAreEqual(TestDotTarGz, StringLit("test.tar"))); 
        Assert(StringsAreEqual(TestDotObjDot, StringLit("test.obj"))); 
        Assert(StringsAreEqual(TestDotDotObj, StringLit("test."))); 
        Assert(StringsAreEqual(Test, StringLit("test"))); 
        Assert(StringsAreEqual(Dot, StringLit(""))); 
        
        Assert(StringsAreEqual(StringLit("file.exe"), FilenameFromPath(StringLit("c:/dev/test/file.exe")))); 
        Assert(StringsAreEqual(StringLit("file.exe"), FilenameFromPath(StringLit("c://dev/test////file.exe")))); 
        Assert(StringsAreEqual(StringLit("file.exe"), FilenameFromPath(StringLit("/c/dev/test/file.exe")))); 
        Assert(StringsAreEqual(StringLit("file"), FilenameFromPath(StringLit("/c/dev/test/file")))); 
        Assert(StringsAreEqual(StringLit("file"), FilenameFromPath(StringLit("/c/.dev/test //file")))); 
        Assert(StringsAreEqual(StringLit(""), FilenameFromPath(StringLit("/c/dev/test/file/")))); 
        Assert(StringsAreEqual(StringLit(""), FilenameFromPath(StringLit("file")))); 
        Assert(StringsAreEqual(StringLit(""), FilenameFromPath(StringLit("/")))); 
        Assert(StringsAreEqual(StringLit("c"), FilenameFromPath(StringLit("/c")))); 
        Assert(StringsAreEqual(StringLit("elmo.doc"), FilenameFromPath(StringLit("c:/abc\\def//hjkl\\\\sdfsdf/elmo.doc")))); 
        
        Assert(StringsAreEqual(StringLit("c:/dev/test/"), DirectoryFromPath(StringLit("c:/dev/test/file.exe")))); 
        Assert(StringsAreEqual(StringLit("c://dev/test////"), DirectoryFromPath(StringLit("c://dev/test////file.exe")))); 
        Assert(StringsAreEqual(StringLit("/c/dev/test/"), DirectoryFromPath(StringLit("/c/dev/test/file.exe")))); 
        Assert(StringsAreEqual(StringLit("/c/dev/test/"), DirectoryFromPath(StringLit("/c/dev/test/file")))); 
        Assert(StringsAreEqual(StringLit("/c/.dev/test //"), DirectoryFromPath(StringLit("/c/.dev/test //file")))); 
        Assert(StringsAreEqual(StringLit("/c/dev/test/file/"), DirectoryFromPath(StringLit("/c/dev/test/file/")))); 
        Assert(StringsAreEqual(StringLit(""), DirectoryFromPath(StringLit("file")))); 
        Assert(StringsAreEqual(StringLit("/"), DirectoryFromPath(StringLit("/")))); 
        Assert(StringsAreEqual(StringLit("/"), DirectoryFromPath(StringLit("/c")))); 
        Assert(StringsAreEqual(StringLit("c:/abc\\def//hjkl\\\\sdfsdf/"), DirectoryFromPath(StringLit("c:/abc\\def//hjkl\\\\sdfsdf/elmo.doc")))); 
        
        temp_memory Scratch = GetScratch();
        
        string_builder Builder = CreateStringBuilder();
        Builder.Append(Scratch.Arena, (char *)"This is the first string. ");
        Builder.Append(Scratch.Arena, StringLit("This is the 2nd string."));
        Builder.Append(Scratch.Arena, StringLit("The last string."));
        Builder.Append(Scratch.Arena, StringLit("\0"));
        
        string WholeString = Builder.GetString(Arena);
        
        ReleaseScratch(Scratch);
        
        Assert(StringsAreEqual(StringPrefix(StringLit("ABCDEF"), 3), StringLit("ABC")));
        Assert(StringsAreEqual(StringPrefix(StringLit("ABCDEF"), 6), StringLit("ABCDEF")));
        Assert(StringsAreEqual(StringPrefix(StringLit("ABCDEF"), 10), StringLit("ABCDEF")));
        Assert(StringsAreEqual(StringPrefix(StringLit("ABCDEF"), 0), StringLit("")));
        Assert(StringsAreEqual(StringPrefix(StringLit(""), 1), StringLit("")));
        
        Assert(StringsAreEqual(StringSuffix(StringLit("ABCDEF"), 3), StringLit("DEF")));
        Assert(StringsAreEqual(StringSuffix(StringLit("ABCDEF"), 5), StringLit("BCDEF")));
        Assert(StringsAreEqual(StringSuffix(StringLit("ABCDEF"), 10), StringLit("ABCDEF")));
        Assert(StringsAreEqual(StringSuffix(StringLit("ABCDEF"), 0), StringLit("")));
        Assert(StringsAreEqual(StringSuffix(StringLit(""), 1), StringLit("")));
        
        Assert(StringsAreEqual(SubstrRange(StringLit("ABCDEF"), 0, 6), StringLit("ABCDEF")));
        Assert(StringsAreEqual(SubstrRange(StringLit("ABCDEF"), 1, 3), StringLit("BC")));
        Assert(StringsAreEqual(SubstrRange(StringLit("ABCDEF"), 3, 3), StringLit("")));
        Assert(StringsAreEqual(SubstrRange(StringLit("ABCDEF"), 3, 4), StringLit("D")));
        Assert(StringsAreEqual(SubstrRange(StringLit("ABCDEF"), 3, 10), StringLit("DEF")));
        Assert(StringsAreEqual(SubstrRange(StringLit("ABCDEF"), 10, 10), StringLit("")));
        Assert(StringsAreEqual(SubstrRange(StringLit(""), 0, 1), StringLit("")));
        
        Assert(StringsAreEqual(Substr(StringLit("ABCDEF"), 4, 1), StringLit("E")));
        Assert(StringsAreEqual(Substr(StringLit("ABCDEF"), 2, 3), StringLit("CDE")));
        Assert(StringsAreEqual(Substr(StringLit("ABCDEF"), 5, 10), StringLit("F")));
        Assert(StringsAreEqual(Substr(StringLit("ABCDEF"), 6, 1), StringLit("")));
        Assert(StringsAreEqual(Substr(StringLit("ABCDEF"), 1, 0), StringLit("")));
        Assert(StringsAreEqual(Substr(StringLit(""), 0, 1), StringLit("")));
        
        FreeArena(Arena);
    }
    
    {
        
        //u8 Contents[] = "lsdkfj;asdlkfj;alskdjf;alskdjf;lkasdjf;l";
        
        // Longest path that I can make a file with, 268 chars including prefix
        //string LongFileName = StringLit("\\\\?\\C:\\files\\ABCDEFGHIJKLMNOPQRSTjkhsdkfjghsdklfjghlsdkjfhglksdjfhglksjdfhglksdjfhglkjdhsfgldksfjghlsdkfjhglkgjhsdlfkjghsldkfjghlsdkfjhglsdkjfhgldfhjgllkdjfhlkdasjhflsdkjfhlasjdhfljkasdhflkjahsdflhasdlkfjhasldkfhalsdkjfhlaskdjhflkasdhflaksdjhfakalaks0123456789abcdefgh");
        
        //Assert(PlatformWriteEntireFile(sizeof(Contents), Contents, LongFileName));
        
        // Doesn't work without prefix
        //Assert(PlatformWriteEntireFile(sizeof(Contents), Contents, StringLit("C:\\files\\ABCDEFGHIJKLMNOPQRSTjkhsdkfjghsdklfjghlsdkjfhglksdjfhglksjdfhglksdjfhglkjdhsfgldksfjghlsdkfjhglkgjhsdlfkjghsldkfjghlsdkfjhglsdkjfhgldfhjgllkdjfhlkdasjhflsdkjfhlasjdhfljkasdhflkjahsdflhasdlkfjhasldkfhalsdkjfhlaskdjhflkasdhflaksdjhfakalaks0123456789abcdefgh")));
        
        //Assert(PlatformDeleteFile(LongFileName));
    }
    {
        arena *Arena = CreateArena(GB(10));
        
        
        string String8 = UTF8FromUTF16(Arena, (wchar_t *)L"Hellow", 6);
        wchar_t *String16 = UTF16FromUTF8(Arena, (u8 *)"Hellow", 6);
        (void)String16;
        
        string ExePath = PlatformGetExecutablePath(Arena);
        Assert(StringsAreEqual(ExePath, StringLit("c:\\dev\\projects\\base\\build\\base_test.exe")));
        
        Assert(PlatformFileExists(StringLit("c:/dev/projects/base/build/test.md")));
        Assert(PlatformFileExists(StringLit("c:/dev/projects/base/build/base_test.exe")));
        Assert(PlatformFileExists(StringLit("c:\\dev\\projects\\base\\build\\base_test.exe")));
        Assert(PlatformFileExists(StringLit("c:\\dev\\projects\\base\\build\\BASE_TEST.EXE")));
        Assert(!PlatformFileExists(StringLit("c:\\dev\\projects\\base\\build\\")));
        Assert(!PlatformFileExists(StringLit("c:\\dev\\projects\\base\\build")));
        Assert(!PlatformFileExists(StringLit("c:\\fake\\fakey")));
        
        
        Assert(PlatformGetFileSize(StringLit("c:\\dev\\projects\\base\\build\\test.md")) == 815);
        
        platform_file_contents File = PlatformReadEntireFile(Arena, StringLit("test.md"));
        
#if 0
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
        
        platform_file_contents LargeFile = PlatformReadEntireFile(Arena, StringLit("large.big"));
        
        DeleteFile("large.big");
#endif
        
        u64 Data[] = { 0x3234234, 0x23423423, 0x0349538450, 0x93213, 0x34882349 };
        Assert(PlatformWriteEntireFile(sizeof(Data), (u8 *)Data,
                                       StringLit("c:/dev/projects/base/build/writefile.test")));
        
        platform_file_contents RoundTripFile = PlatformReadEntireFile(Arena, StringLit("c:/dev/projects/base/build/writefile.test"));
        Assert(RoundTripFile.Size == sizeof(Data));
        Assert(memcmp(Data, RoundTripFile.Contents, sizeof(Data)) == 0);
        
        Assert(PlatformDeleteFile(StringLit("c:/dev/projects/base/build/writefile.test")));
        
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
    }
}
