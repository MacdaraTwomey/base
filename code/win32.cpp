
#include "base.h"
#include "platform.h"
#include "base.cpp"
#include "ring_buffer.h"

#include <windows.h>

static LARGE_INTEGER GlobalPerformanceFrequency = {};

//
// TODO:
// - PLATFORM_ERROR_MESSAGE to make message box
// - Our own custom utf8 conversion code

HANDLE Win32CreateConsole()
{
    AllocConsole();
    
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONIN", "r", stdin);
    
    HANDLE con_out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD out_mode;
    GetConsoleMode(con_out, &out_mode);
    
    return GetStdHandle(STD_INPUT_HANDLE);
}

s64 Win32GetTime()
{
    LARGE_INTEGER Now;
    QueryPerformanceCounter(&Now);
    return Now.QuadPart;
}

s64 Win32GetMicrosecondsElaped(s64 start, s64 end)
{
    s64 Microseconds;
    Microseconds = end - start;
    Microseconds *= 1000000; // microseconds per second
    Microseconds /= GlobalPerformanceFrequency.QuadPart; // ticks per second
    return Microseconds;
}

// Returns non-null terminated string
string UTF8FromUTF16(arena *Arena, wchar_t *String16, u32 String16Length)
{
    Assert(String16Length > 0);
    
    u32 Capacity = (String16Length * 4);
    u8 *Buffer = PushArrayZero(Arena, Capacity, u8);
    
    // Returns 0 on failure
    int ByteCount = WideCharToMultiByte(CP_UTF8, 0, 
                                        String16, (int)String16Length, 
                                        (char *)Buffer, (int)Capacity, 
                                        NULL, NULL);
    PopSize(Arena, Capacity - ByteCount);
    return CreateString(Buffer, (u64)ByteCount);
}

// Returns null-terminated string
wchar_t *UTF16FromUTF8(arena *Arena, u8 *String8, int String8Length)
{
    Assert(String8Length > 0);
    
    int Capacity = (String8Length * 2) + 2;
    wchar_t *Buffer = PushArrayZero(Arena, Capacity, wchar_t);
    
    int CharacterCount = MultiByteToWideChar(CP_UTF8, 0, 
                                             (char *)String8, String8Length, 
                                             Buffer, Capacity);
    Buffer[CharacterCount] = 0;
    return Buffer;
}

string PlatformGetExecutablePath(arena *Arena)
{
    temp_memory Scratch = GetScratch();
    
    string Result = {};
    
    DWORD Capacity = 1024;
    wchar_t *Buffer = PushArrayZero(Scratch.Arena, Capacity, wchar_t);
    
    u32 Size = 0;
    for (int Tries = 0; Tries < 4; ++Tries)
    {
        // If succeeds then Size is the length NOT including the null terminator.
        // If truncates then Size is the truncated length including the null terminator.
        // If fails then Size is zero.
        Size = GetModuleFileNameW(NULL, Buffer, Capacity);
        if (Size == Capacity)
        {
            // Truncated (Size == NullTerminatedStringSize)
            Capacity *= 2;
            Buffer = PushArrayZero(Scratch.Arena, Capacity, wchar_t);
        }
        else if (Size == 0)
        {
            // Error 
            break;
        }
        else if (Size < Capacity)
        {
            // Success (Size == NonNullTerminatedFileNameLength)
            Result = UTF8FromUTF16(Arena, Buffer, Size);
            break;
        }
    }
    
    ReleaseScratch(Scratch);
    
    return Result;
}

bool PlatformFileExists(string FilePath)
{
    // I think you need to prepend \\?\ to exceed 260 Characters
    // TODO: Test with a long path
    // This null terminates for us
    temp_memory Scratch = GetScratch();
    
    wchar_t *FilePath16 = UTF16FromUTF8(Scratch.Arena, FilePath.Str, (int)FilePath.Length);
    DWORD Result = GetFileAttributesW(FilePath16);
    bool IsDirectory = Result & FILE_ATTRIBUTE_DIRECTORY;
    bool Exists = Result != INVALID_FILE_ATTRIBUTES; 
    
    ReleaseScratch(Scratch);
    
    return Exists && !IsDirectory;
}

u64 PlatformGetFileSize(string FilePath)
{
    u64 Result = 0;
    
    temp_memory Scratch = GetScratch();
    
    wchar_t *FilePath16 = UTF16FromUTF8(Scratch.Arena, FilePath.Str, (int)FilePath.Length);
    
    HANDLE File = CreateFileW(FilePath16, GENERIC_READ, FILE_SHARE_READ, NULL, 
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (File != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER Size;
        if (GetFileSizeEx(File, &Size))
        {
            Result = (u64)Size.QuadPart;
        }
    }
    
    ReleaseScratch(Scratch);
    
    return Result;
}

platform_file_contents PlatformReadEntireFile(arena *Arena, string FilePath)
{
    platform_file_contents Result = {};
    
    temp_memory Scratch = GetScratch();
    
    wchar_t *FilePath16 = UTF16FromUTF8(Scratch.Arena, FilePath.Str, (int)FilePath.Length);
    
    HANDLE File = CreateFileW(FilePath16, GENERIC_READ, FILE_SHARE_READ, NULL, 
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (File != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER Size;
        if (GetFileSizeEx(File, &Size))
        {
            u64 FileSize = (u64)Size.QuadPart;
            if (FileSize > 0)
            {
                u8 *Contents = PushArray(Arena, FileSize, u8);
                if (Contents)
                {
                    u8 *Dest = Contents;
                    u64 BytesRemaining = FileSize;
                    while (BytesRemaining > 0)
                    {
                        DWORD ChunkSize = (DWORD)ClampTop(BytesRemaining, UINT32_MAX);
                        DWORD BytesRead = 0;
                        if (ReadFile(File, Dest, ChunkSize, &BytesRead, 0))
                        {
                            BytesRemaining -= BytesRead;
                            Dest += BytesRead;
                        }
                        else
                        {
                            break;
                        }
                    }
                    
                    if (BytesRemaining == 0)
                    {
                        Result.Contents = Contents;
                        Result.Size = FileSize;
                    }
                    else
                    {
                        PopSize(Arena, FileSize);
                    }
                }
            }
        }
        
        CloseHandle(File);
    }
    
    ReleaseScratch(Scratch);
    
    return Result;
}

// Creates a file or replaces an existing file.
bool PlatformWriteEntireFile(u64 Size, u8 *Contents, string FilePath)
{
    temp_memory Scratch = GetScratch();
    
    bool Success = false;
    
    wchar_t *FilePath16 = UTF16FromUTF8(Scratch.Arena, FilePath.Str, (int)FilePath.Length);
    
    HANDLE File = CreateFileW(FilePath16, GENERIC_WRITE, 0, NULL, 
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (File != INVALID_HANDLE_VALUE)
    {
        u8 *Source = Contents;
        u64 BytesRemaining = Size;
        while (BytesRemaining > 0)
        {
            DWORD ChunkSize = (DWORD)ClampTop(BytesRemaining, UINT32_MAX);
            DWORD BytesWritten = 0;
            if (WriteFile(File, Source, ChunkSize, &BytesWritten, 0))
            {
                BytesRemaining -= BytesWritten;
                Source += BytesWritten;
            }
            else
            {
                break;
            }
        }
        
        if (BytesRemaining == 0)
        {
            Success = true;
        }
        
        CloseHandle(File);
    }
    
    ReleaseScratch(Scratch);
    
    return Success;
}

bool PlatformDeleteFile(string FilePath)
{
    temp_memory Scratch = GetScratch();
    
    wchar_t *FilePath16 = UTF16FromUTF8(Scratch.Arena, FilePath.Str, (int)FilePath.Length);
    bool Success = (DeleteFileW(FilePath16) != 0);
    
    ReleaseScratch(Scratch);
    
    return Success;
}

void *PlatformMemoryReserve(u64 Size)
{
    Assert(Size > 0);
    void *Memory = VirtualAlloc(0, Size, MEM_RESERVE, PAGE_NOACCESS);
    return Memory;
}

bool PlatformMemoryCommit(void *Address, u64 Size)
{
    // Can succeed even if Address is NULL
    // Does not fail if you try to commit an already commited page
    // Actual physical pages are not allocated until they are accessed
    // Zeros the commited pages
    Assert(Address);
    Assert(Size > 0);
    bool Success = (VirtualAlloc(Address, Size, MEM_COMMIT, PAGE_READWRITE) != 0);
    return Success;
}

void PlatformMemoryDecommit(void *Address, u64 Size)
{
    // Frees all pages with a byte in the range from Address to Address+Size
    Assert(Address);
    Assert(Size > 0);
    // Does not fail if you try to decommit an uncommited page
    BOOL Success = VirtualFree(Address, Size, MEM_DECOMMIT);
    Assert(Success);
}

void PlatformMemoryFree(void *Address)
{
    // Decommits all commited pages and releases reserved memory region
    Assert(Address);
    BOOL Success = VirtualFree(Address, 0, MEM_RELEASE);
    Assert(Success);
}

void PlatformMemoryGuard(void *Address, u64 Size)
{
    // Only for debug builds
    
    // Undefined behaviour 
    Assert(Address);
    u64 AddressInt = (u64)Address;
    Assert((AddressInt & (4096 - 1)) == 0);
    Assert((Size & (4096 - 1)) == 0);
    
    // We aren't using PAGE_GUARD as that throws an exception, then reverts to the old protection.
    // PAGE_NOACCESS just throws the access violation.
    DWORD OldProtectFlags;
    BOOL Success = VirtualProtect(Address, Size, PAGE_NOACCESS, &OldProtectFlags);
    Assert(Success);
}

void PlatformMemoryRemoveGuard(void *Address, u64 Size)
{
    // Only for debug builds
    
    // Undefined behaviour 
    Assert(Address);
    u64 AddressInt = (u64)Address;
    Assert((AddressInt & (4096 - 1)) == 0);
    Assert((Size & (4096 - 1)) == 0);
    
    DWORD OldProtectFlags;
    BOOL Success = VirtualProtect(Address, Size, PAGE_READWRITE, &OldProtectFlags);
    Assert(Success);
}

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

#if 0
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


struct test
{
    u32 A;
    u32 B;
    u32 C;
    u64 D;
};

static bool GlobalAppRunning = false;

LRESULT CALLBACK Win32WindowProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
    LRESULT Result = 0;
    switch (Message)
    {
        case WM_CLOSE:
        {
            GlobalAppRunning = false;
        } break;
        
        case WM_DESTROY:
        {
            GlobalAppRunning = false;
        } break;
        
        case WM_MOUSEWHEEL: 
        case WM_MOUSEMOVE: 
        case WM_LBUTTONUP: 
        case WM_LBUTTONDOWN: 
        case WM_LBUTTONDBLCLK: 
        case WM_RBUTTONUP: 
        case WM_RBUTTONDBLCLK: 
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
        }
        
        default:
        {
            Result = DefWindowProcA(Window, Message, wParam, lParam);
        } break;
    }
    
    return Result;
}

#include <gl/gl.h> // Windows OpenGL 1.1

// glcorearb.h includes only APIs from the latest core profile and some newer ARB extensions that are supported.
// it does not include functionality removed from the core.
// Don't include glcorearb.h with either gl/gl.h or glext.h

// glext.h contains APIs from OPENGL 1.2 and above

// wglext.h contains WGL extension interfaces
#include "../deps/GL/wglext.h" // TODO: Pull out the defines from this then remove the header file

// The OpenGL headers now depend on the KHR/khrplatform.h header

HMODULE OpenGL32DLL = nullptr;

#define OpenGLFunction(Name, Type) PFN##Type##PROC Name = nullptr

//#define OpenGLFunction(Name, Type) Name = PlatformLoadOpenGLFunction(#Name)









// TODO: Does glew fall back to GetProcAddress
// TODO: Test if OpenGL1.1 functions fail to load by wglGetProcAddress
void *PlatformLoadOpenGLFunction(const char *Name)
{
    Assert(wglGetProcAddress);
    Assert(OpenGL32DLL);
    void *Result = (void *)wglGetProcAddress(Name);
    if (Result == 0 ||
        (Result == (void*)0x1) || (Result == (void*)0x2) || (Result == (void*)0x3) ||
        (Result == (void*)-1))
    {
        // wglGetProcAddress will not return pointers to functions exported by opengl32.dll
        Result = (void *)GetProcAddress(OpenGL32DLL, Name);
    }
    
    return Result;
}

void Win32LoadWGLExtensions()
{
    
}

void Win32SetupOpenGLContext()
{
    WNDCLASSA DummyClass = {};
    DummyClass.lpfnWndProc = DefWindowProcA;
    DummyClass.hInstance = hInstance;
    DummyClass.lpszClassName = "DummyWindowClass";
    if (RegisterClassA(&DummyClass))
    {
        HWND DummyWindow = CreateWindowExA(0, 
                                           DummyClass.lpszClassName,
                                           "Base",
                                           WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
                                           CW_USEDEFAULT, CW_USEDEFAULT,
                                           10, 10,
                                           NULL, NULL, 
                                           hInstance, 
                                           NULL);
        if (DummyWindow)
        {
            // TODO: FreeDC()?
            HDC DummyDC = GetDC(DummyWindow);
            
            PIXELFORMATDESCRIPTOR DummyFormat = {};
            DummyFormat.nSize = sizeof(PIXELFORMATDESCRIPTOR);
            DummyFormat.nVersion = 1;
            DummyFormat.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
            DummyFormat.iPixelType = PFD_TYPE_RGBA;
            DummyFormat.cColorBits = 32; // TODO: CHECK if GLEW sets this to 32 or 24
            DummyFormat.cAlphaBits = 8;
            DummyFormat.cDepthBits = 24; // NOTE: 24-bit depth buffer
            
            int PixelFormatIndex = ChoosePixelFormat(DummyDC, &DummyFormat);
            if (PixelFormatIndex > 0)
            {
                // TODO: Does GLEW Skip this step
                PIXELFORMATDESCRIPTOR DescribedFormat = {};
                int DescribeResult = DescribePixelFormat(DummyDC, PixelFormatIndex, 
                                                         sizeof(DescribedFormat), &DescribedFormat);
                Assert(DescribeResult != 0);
                
                // We can only set a pixel format on a window once, hence the dummy window
                BOOL SetFormat = SetPixelFormat(DummyDC, PixelFormatIndex, &DescribedFormat);
                if (SetFormat)
                {
                    HGLRC DummyRC = wglCreateContext(DummyDC);
                    if (DummyRC)
                    {
                        if (wglMakeCurrent(DummyDC, DummyRC))
                        {
                            PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC) wglGetProcAddress("wglChoosePixelFormatARB");
                            
                            PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = 
                            (PFNWGLCREATECONTEXTATTRIBSARBPROC) wglGetProcAddress("wglCreateContextAttribsARB");
                            
                            PFNWGLMAKECONTEXTCURRENTARBPROC wglMakeContextCurrentARB = 
                            (PFNWGLMAKECONTEXTCURRENTARBPROC) wglGetProcAddress("wglMakeContextCurrentARB");
                            
                            PFNWGLGETEXTENSIONSSTRINGEXTPROC wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)  wglGetProcAddress("wglGetExtensionsStringEXT"); 
                            
                            PFWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = 
                            (PFWGLSWAPINTERVALEXTPROC) wglGetProcAddress("wglSwapIntervalEXT");
                            
                            bool sRGBFrameBufferAvailable = false;
                            if (wglGetExtensionsStringEXT)
                            {
                                // The contents of the string is implementation specific, the string will be NULL terminated and will contain a space-separated list of extension names. (The extension names themselves do not contain spaces.) If there are no extensions then the empty string is returned.
                                // Returns NULL on failure
                                const char *WGLExtensionsString = wglGetExtensionsStringEXT();
                                if (WGLExtensionsString)
                                {
                                    string ExtensionList = CreateString((u8 *)WGLExtensionsString);
                                    while (ExtensionList.Length > 0)
                                    {
                                        u64 SpaceIndex = StringFindChar(ExtensionList, ' ');
                                        if (SpaceIndex < ExtensionList.Length)
                                        {
                                            string Extension = StringPrefix(ExtensionList, SpaceIndex);
                                            if (StringsAreEqual(Extension, StringLit("WGL_EXT_framebuffer_sRGB")) ||
                                                StringsAreEqual(Extension, StringLit("WGL_ARB_framebuffer_sRGB")))
                                            {
                                                sRGBFrameBufferAvailable = true;
                                                break;
                                            }
                                        }
                                        
                                        ExtensionList = StringSkip(ExtensionList, SpaceIndex);
                                    }
                                }
                            }
                            
                            // TODO: FreeDC()?
                            HDC WindowDC = GetDC(Window);
                            
                            int sRGBFrameBufferCapable = (sRGBFrameBufferAvailable) ? GL_TRUE : GL_FALSE;
                            int AttributeList[] = {
                                WGL_DRAW_TO_WINDOW_ARB,  GL_TRUE,
                                WGL_SUPPORT_OPENGL_ARB,  GL_TRUE,
                                WGL_DOUBLE_BUFFER_ARB,   GL_TRUE,
                                WGL_PIXEL_TYPE_ARB,      WGL_TYPE_RGBA_ARB,
                                WGL_ACCELERATION_ARB,    WGL_FULL_ACCELERATION_ARB,
                                WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, sRGBFrameBufferAvailable,
                                WGL_COLOR_BITS_ARB,      32,   // TODO: Should be 24?
                                WGL_ALPHA_BITS_ARB,      8,
                                WGL_DEPTH_BITS_ARB,      24,
                                WGL_STENCIL_BITS_ARB,    8,
                                WGL_SAMPLE_BUFFERS_ARB,  1, // Minumum of 1 multisample buffers
                                WGL_SAMPLES_ARB,         4, // Number of samples per pixel
                                0 // Zero terminates list
                            };
                            
                            int PixelFormatIndex = 0;
                            int FormatCount = 0;
                            BOOL FoundPixelFormat = wglChoosePixelFormatARB(WindowDC, AttributeList, NULL, 1, &PixelFormatIndex, &FormatCount);
                            if (FoundPixelFormat && FormatCount == 1)
                            {
                                PIXELFORMATDESCRIPTOR PixelFormat = {};
                                int FormatDescribeResult = DescribePixelFormat(WindowDC, PixelFormatIndex, sizeof(PixelFormat), &PixelFormat);
                                Assert(FormatDescribeResult != 1);
                                
                                BOOL SetFormatResult = SetPixelFormat(WindowDC, PixelFormatIndex, &PixelFormat);
                                if (SetFormatResult)
                                {
                                    int ContextAttributes[] = {
                                        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
                                        WGL_CONTEXT_MINOR_VERSION_ARB, 1,
                                        WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                                        WGL_CONTEXT_FLAGS_ARB,         WGL_CONTEXT_DEBUG_BIT_ARB,   // NOTE: Debug context
                                        0
                                    };
                                    
                                    // The returned context may implement the version requested OR a greater version.
                                    // Query the actual version with glGetString.
                                    if (wglCreateContextAttribsARB)
                                    {
                                        // If we can't create a modern context then we don't try to use a older context.
                                        HGLRC WindowRC = wglCreateContextAttribsARB(WindowDC, 0, ContextAttributes);
                                        if (WindowRC)
                                        {
                                            wglMakeCurrent(NULL, NULL);
                                            wglDeleteContext(DummyRC);
                                            ReleaseDC(DummyDC);
                                            DestroyWindow(DummyWindow);
                                            
                                            if (wglMakeCurrent(WindowRC))
                                            {
                                                
                                            }
                                        }
                                    }
                                }
                            }
                            
                            ReleaseDC(WindowDC);
                        }
                    }
                }
            }
        }
    }
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)pCmdLine;
    (void)nCmdShow;
    
    //Win32CreateConsole();
    
    // We load this so we can call glGetProcAddress on functions that fail
    // wglGetProcAddress
    HMODULE OpenGLDLL = LoadLibraryA("opengl.dll");
    if (OpenGLDLL)
    {
    }
    
    int WindowWidth = 720;
    int WindowHeight = 480;
    
    // TODO: DO we need to call AdjustWindowRect for correctly sized window because of border stuff?
    // Create real window
    // 
    
    
    
    // Create real window
    WNDCLASSA WindowClass = {};
    WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    WindowClass.lpfnWndProc = Win32WindowProc;
    WindowClass.hInstance = hInstance;
    WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    WindowClass.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH); // TODO: Make into black brush
    WindowClass.lpszClassName = "MainWindowClass";
    if (RegisterClassA(&WindowClass))
    {
        HWND Window = CreateWindowExA(0, 
                                      WindowClass.lpszClassName,
                                      "Base",
                                      WS_OVERLAPPEDWINDOW,
                                      CW_USEDEFAULT, CW_USEDEFAULT,
                                      WindowWidth, WindowHeight,
                                      NULL, NULL, 
                                      hInstance, 
                                      NULL);
        if (Window)
        {
            
            
            
            
            
        }
    }
    
    if (Window)
    {
        
    }
}





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

return 0;
}

