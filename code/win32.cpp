
#include "base.h"
#include "platform.h"
#include "base.cpp"
#include "ring_buffer.h"

#include <windows.h>

#include "GL/glcorearb.h"
#include "GL/wglext.h"  // wglext.h contains WGL extension interfaces

#include "opengl.cpp"

#include "test.cpp"

static LARGE_INTEGER GlobalPerformanceFrequency = {};

static bool GlobalAppRunning = false;

static HMODULE OpenGL32DLL = nullptr;

//
// TODO:
// - PLATFORM_ERROR_MESSAGE to make message box?

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
                        // Success
                        Result.Contents = Contents;
                        Result.Size = FileSize;
                    }
                    else
                    {
                        // Error
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
        
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            WPARAM VKCode = wParam;
            if (VKCode == VK_ESCAPE)
            {
                GlobalAppRunning = false;
            }
        }
        
        default:
        {
            Result = DefWindowProcA(Window, Message, wParam, lParam);
        } break;
    }
    
    return Result;
}

void *Win32LoadOpenGLFunction(const char *Name)
{
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

void SetSwapInterval(int Interval)
{
    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress("wglSwapIntervalEXT");
    if (wglSwapIntervalEXT)
    {
        wglSwapIntervalEXT(Interval);
    }
}

bool SetDummyPixelFormat(HDC DummyDC)
{
    bool Success = false;
    
    PIXELFORMATDESCRIPTOR DummyFormat = {};
    DummyFormat.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    DummyFormat.nVersion = 1;
    DummyFormat.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    DummyFormat.iPixelType = PFD_TYPE_RGBA;
    DummyFormat.cColorBits = 24; 
    DummyFormat.cAlphaBits = 8;
    DummyFormat.cDepthBits = 24; 
    
    int PixelFormatIndex = ChoosePixelFormat(DummyDC, &DummyFormat);
    if (PixelFormatIndex > 0)
    {
        PIXELFORMATDESCRIPTOR DescribedFormat = {};
        if (DescribePixelFormat(DummyDC, PixelFormatIndex, sizeof(DescribedFormat), &DescribedFormat))
        {
            if (SetPixelFormat(DummyDC, PixelFormatIndex, &DescribedFormat))
            {
                Success = true;
            }
        }
    }
    
    return Success;
}

bool SetRealPixelFormat(HDC WindowDC, bool sRGBSupported)
{
    bool Success = false;
    
    int AttributeList[] = {
        WGL_DRAW_TO_WINDOW_ARB,  GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB,  GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB,   GL_TRUE,
        WGL_PIXEL_TYPE_ARB,      WGL_TYPE_RGBA_ARB,
        WGL_ACCELERATION_ARB,    WGL_FULL_ACCELERATION_ARB,
        WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, ((sRGBSupported) ? GL_TRUE : GL_FALSE),
        WGL_COLOR_BITS_ARB,      24,  // DescribePixelFormat turns this to 32, but docs say it should be 32...
        WGL_ALPHA_BITS_ARB,      8,
        WGL_DEPTH_BITS_ARB,      24,
        WGL_STENCIL_BITS_ARB,    8,
        WGL_SAMPLE_BUFFERS_ARB,  1, // Minumum of 1 multisample buffers
        WGL_SAMPLES_ARB,         4, // Number of samples per pixel
        0 // Zero terminates list
    };
    
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC) wglGetProcAddress("wglChoosePixelFormatARB");
    if (wglChoosePixelFormatARB)
    {
        int PixelFormatIndex = 0;
        unsigned int FormatCount = 0;
        if (wglChoosePixelFormatARB(WindowDC, AttributeList, NULL, 1, &PixelFormatIndex, &FormatCount))
        {
            PIXELFORMATDESCRIPTOR PixelFormat = {};
            if (DescribePixelFormat(WindowDC, PixelFormatIndex, sizeof(PixelFormat), &PixelFormat))
            {
                if (SetPixelFormat(WindowDC, PixelFormatIndex, &PixelFormat))
                {
                    Success = true;
                }
            }
        }
    }
    
    return Success;
}

// The returned context may implement the version requested OR a greater version.
// Query the actual version with glGetString.
// If we can't create a modern context then we don't try to use a older context.
// NOTE: This function is saying we will write both sRGB and non-sRGB code paths later
bool Win32CreateOpenGLContext(HDC WindowDC)
{
    bool Success = false;
    
    WNDCLASSA DummyClass = {};
    DummyClass.lpfnWndProc = DefWindowProcA;
    DummyClass.hInstance = GetModuleHandle(NULL);
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
                                           DummyClass.hInstance, 
                                           NULL);
        if (DummyWindow)
        {
            HDC DummyDC = GetDC(DummyWindow);
            if (SetDummyPixelFormat(DummyDC))
            {
                HGLRC DummyRC = wglCreateContext(DummyDC);
                if (DummyRC)
                {
                    if (wglMakeCurrent(DummyDC, DummyRC))
                    {
                        bool sRGBSupported = false;
                        
                        PFNWGLGETEXTENSIONSSTRINGEXTPROC wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)  wglGetProcAddress("wglGetExtensionsStringEXT"); 
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
                                            sRGBSupported = true;
                                            break;
                                        }
                                    }
                                    
                                    ExtensionList = StringSkip(ExtensionList, SpaceIndex + 1);
                                }
                            }
                        }
                        
                        if (SetRealPixelFormat(WindowDC, sRGBSupported))
                        {
                            int ContextAttributes[] = {
                                WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
                                WGL_CONTEXT_MINOR_VERSION_ARB, 2,
                                WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                                WGL_CONTEXT_FLAGS_ARB,         WGL_CONTEXT_DEBUG_BIT_ARB,   // NOTE: Debug context
                                0
                            };
                            
                            PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = 
                            (PFNWGLCREATECONTEXTATTRIBSARBPROC) wglGetProcAddress("wglCreateContextAttribsARB");
                            if (wglCreateContextAttribsARB)
                            {
                                HGLRC WindowRC = wglCreateContextAttribsARB(WindowDC, 0, ContextAttributes);
                                if (WindowRC)
                                {
                                    wglMakeCurrent(NULL, NULL);
                                    if (wglMakeCurrent(WindowDC, WindowRC))
                                    {
                                        Success = true;
                                    }
                                }
                            }
                        }
                        wglDeleteContext(DummyRC);
                    }
                }
                ReleaseDC(DummyWindow, DummyDC);
            }
            DestroyWindow(DummyWindow);
        }
    }
    
    return Success;
}

void Win32LoadOpenGLFunctions()
{
#define OpenGLFunction(Name, Type) gl##Name = (PFNGL##Type##PROC) Win32LoadOpenGLFunction("gl"#Name);
#include "opengl_functions.h"
#undef OpenGLFunction
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)pCmdLine;
    (void)nCmdShow;
    
    Win32CreateConsole();
    
    int WindowWidth = 1280;
    int WindowHeight = 720;
    
    // TODO: DO we need to call AdjustWindowRect for correctly sized window because of border stuff?
    // Create real window
    // 
    
    WNDCLASSA WindowClass = {};
    WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    WindowClass.lpfnWndProc = Win32WindowProc;
    WindowClass.hInstance = hInstance;
    WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    WindowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); 
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
            HDC WindowDC = GetDC(Window);
            if (Win32CreateOpenGLContext(WindowDC))
            {
                ShowWindow(Window, nCmdShow);
                
                OpenGL32DLL = LoadLibraryA("opengl32.dll"); 
                if (OpenGL32DLL)
                {
                    SetSwapInterval(1);
                    Win32LoadOpenGLFunctions();
                }
            }
            
            ReleaseDC(Window, WindowDC);
        }
    }
    
    RunTests();
    
    InitOpenGL();
    
    GlobalAppRunning = true;
    while (GlobalAppRunning)
    {
        MSG Message = {};
        while (true)
        {
            if (PeekMessage(&Message, NULL, NULL, NULL, PM_REMOVE))
            {
                TranslateMessage(&Message);
                DispatchMessageA(&Message);
            }
            else
            {
                break;
            }
        }
        
        OpenGLEndFrame(WindowWidth, WindowHeight);
        
        SwapBuffers(wglGetCurrentDC());
    }
    
    
    
    
    return 0;
}

