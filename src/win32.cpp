
#include "base.h"
#include "platform.h"
#include "base.cpp"

#include "win32_opengl.cpp"
#include "opengl.cpp"

static LARGE_INTEGER GlobalPerformanceFrequency = {};

static bool GlobalAppRunning = false;

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
    u8 *Buffer = PushArray(Arena, Capacity, u8);
    
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
    wchar_t *Buffer = PushArray(Arena, Capacity, wchar_t);
    
    int CharacterCount = MultiByteToWideChar(CP_UTF8, 0, 
                                             (char *)String8, String8Length, 
                                             Buffer, Capacity);
    Buffer[CharacterCount] = 0;
    return Buffer;
}

string PlatformGetExecutablePath(arena *Arena)
{
    temp_arena Scratch = GetScratch(Arena);
    
    string Result = {};
    
    DWORD Capacity = 1024;
    wchar_t *Buffer = PushArray(Scratch.Arena, Capacity, wchar_t);
    
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
            Buffer = PushArray(Scratch.Arena, Capacity, wchar_t);
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

string PlatformGetExecutablePath(arena *Arena)
{
    string Path = {};
    
    temp_arena Scratch = GetScratch(Arena);
    
    u64 BufferSize = KB(8);
    u8 *Buffer = PushArrayNoZero(Scratch.Arena, BufferSize, u8);
    // If succeeds then Size is the length NOT including the null terminator.
    // If truncates then Size is the truncated length including the null terminator.
    // If fails then Size is zero.
    u32 Size = GetModuleFileNameW(NULL, Buffer, Capacity);
    if (Size < BufferSize)
    {
        Path = UTF8FromUTF16(Arena, Buffer, Size);
    }
    
    ReleaseScratch(Scratch);
    
    return Path;
}

// TODO: Maybe allow this to return true if the file is a directory
bool PlatformFileExists(string FilePath)
{
    temp_arena Scratch = GetScratch();
    
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
    
    temp_arena Scratch = GetScratch();
    
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

string PlatformReadEntireFile(arena *Arena, string FilePath)
{
    string Contents = {};
    
    temp_arena Scratch = GetScratch(Arena);
    
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
                u8 *FileData = PushArrayNoZero(Arena, FileSize, u8);
                if (FileData)
                {
                    u8 *Dest = FileData;
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
                        Contents.Str = FileData;
                        Contents.Length = FileSize;
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
    
    return Contents;
}

// Creates a file or replaces an existing file.
bool PlatformWriteEntireFile(u64 Size, u8 *Contents, string FilePath)
{
    temp_arena Scratch = GetScratch();
    
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

// TODO: Handle deleteing directories
bool PlatformDeleteFile(string FilePath)
{
    temp_arena Scratch = GetScratch();
    
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

void PlatformMemoryFree(void *Address, u64)
{
    // Decommits all commited pages and releases reserved memory region
    Assert(Address);
    BOOL Success = VirtualFree(Address, 0, MEM_RELEASE);
    Assert(Success);
}

void PlatformMemoryGuard(void *Address, u64 Size)
{
    Assert(Address);
    Assert(Size > 0);
    Assert(((uintptr_t)Address & (4096 - 1)) == 0);
    Assert((Size & (4096 - 1)) == 0);
    
    // We aren't using PAGE_GUARD as that throws an exception, then reverts to the old protection.
    // PAGE_NOACCESS just throws the access violation.
    DWORD OldProtectFlags;
    BOOL Success = VirtualProtect(Address, Size, PAGE_NOACCESS, &OldProtectFlags);
    Assert(Success);
}

void PlatformMemoryRemoveGuard(void *Address, u64 Size)
{
    Assert(Address);
    Assert(Size > 0);
    Assert(((uintptr_t)Address & (4096 - 1)) == 0);
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

v2s Win32GetWindowDim(HWND Window)
{
    RECT ClientDim = {};
    GetClientRect(Window, &ClientDim);
    v2s Result = {
        ClientDim.right - ClientDim.left,
        ClientDim.bottom - ClientDim.top
    };
    
    return Result;
}


int main(int ArgCount, char *Args[]) {
    AppMain();
}

#if 0
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    Win32CreateConsole();
    (void)hInstance;
    (void)hPrevInstance;
    (void)pCmdLine;
    (void)nCmdShow;
    
    HWND Window = 0;
    
    WNDCLASSA WindowClass = {};
    WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;  
    WindowClass.lpfnWndProc = Win32WindowProc;
    WindowClass.hInstance = hInstance;
    WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    WindowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); 
    WindowClass.lpszClassName = "MainWindowClass";
    if (RegisterClassA(&WindowClass))
    {
        int WindowWidth = 1280;
        int WindowHeight = 1000;
        
        DWORD WindowStyle = WS_OVERLAPPEDWINDOW;
        // Calculate the size of the window so our client area is what we actually want. 
        // It seems like this may not consistantly work and we may need to different system for fullscreen.
        RECT Rect = {0, 0, WindowWidth, WindowHeight};
        AdjustWindowRect(&Rect, WindowStyle, false);
        
        int RequestedWindowWidth = Rect.right - Rect.left;
        int RequestedWindowHeight = Rect.bottom - Rect.top;
        
        Window = CreateWindowExA(0, 
                                 WindowClass.lpszClassName,
                                 "Base",
                                 WindowStyle,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 RequestedWindowWidth, RequestedWindowHeight,
                                 NULL, NULL, 
                                 hInstance, 
                                 NULL);
        if (Window)
        {
            HDC WindowDC = GetDC(Window);
            if (Win32InitOpenGL(WindowDC))
            {
                ShowWindow(Window, nCmdShow);
                
                InitOpenGL();
                
                GlobalAppRunning = true;
                while (GlobalAppRunning)
                {
                    v2s WindowDim = Win32GetWindowDim(Window);
                    OpenGLBeginFrame();
                    
                    OpenGLEndFrame(WindowDim.Width, WindowDim.Height);
                    
                    SwapBuffers(WindowDC);
                }
                
            }
            
            ReleaseDC(Window, WindowDC);
        }
    }
}
#endif

