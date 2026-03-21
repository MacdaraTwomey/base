
#include "base.h"
#include "os.h"

#include "win32_opengl.cpp"

static LARGE_INTEGER GlobalPerformanceFrequency = {};
static HINSTANCE GlobalInstanceHandle = 0;
static os_key_map GlobalKeyMap = {};

//
// TODO:
// - OS__ERROR_MESSAGE to make message box?

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// Filesystem
//

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


// TODO: Maybe allow this to return true if the file is a directory
bool OS_FileExists(string FilePath)
{
    temp_arena Scratch = GetScratch();
    
    wchar_t *FilePath16 = UTF16FromUTF8(Scratch.Arena, FilePath.Str, (int)FilePath.Length);
    DWORD Result = GetFileAttributesW(FilePath16);
    bool IsDirectory = Result & FILE_ATTRIBUTE_DIRECTORY;
    bool Exists = Result != INVALID_FILE_ATTRIBUTES; 
    
    ReleaseScratch(Scratch);
    
    return Exists && !IsDirectory;
}

os_file_info OS_GetFileInfo(string Path)
{
    Assert(!"Unimplemented for windows");
    return OS_FileInfo_Error;
}

u64 OS_GetFileSize(string FilePath)
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

string OS_GetExecutablePath(arena *Arena)
{
    string Path = {};
    
    temp_arena Scratch = GetScratch(Arena);
    
    u32 Capacity = KB(4);
    wchar_t *Buffer = PushArrayNoZero(Scratch.Arena, Capacity, wchar_t);
    // If succeeds then Size is the length NOT including the null terminator.
    // If truncates then Size is the truncated length including the null terminator.
    // If fails then Size is zero.
    u32 Size = GetModuleFileNameW(0, (wchar_t *)Buffer, Capacity);
    if (Size < Capacity)
    {
        Path = UTF8FromUTF16(Arena, Buffer, Size);
    }
    
    ReleaseScratch(Scratch);
    
    return Path;
}

string OS_CanonicalAbsolutePath(arena *Arena, string Path)
{
    Assert(!"Unimplemented for windows");
    return string{};
}

os_directory_iterator OS_DirectoryIterator(string DirectoryPath)
{
    Assert(!"Unimplemented for windows");
    return os_directory_iterator{};
}

void OS_DirectoryIteratorClose(os_directory_iterator *Iter)
{
    Assert(!"Unimplemented for windows");
}

bool OS_DirectoryIteratorIsOk(os_directory_iterator Iter)
{
    Assert(!"Unimplemented for windows");
    return false;
}

os_filesystem_entry OS_DirectoryIteratorNext(arena *Arena, os_directory_iterator *Iter)
{
    Assert(!"Unimplemented for windows");
    return os_filesystem_entry{};
}

string OS_ReadEntireFile(arena *Arena, string FilePath)
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
bool OS_WriteEntireFile(u64 Size, u8 *Contents, string FilePath)
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
bool OS_DeleteFile(string FilePath)
{
    temp_arena Scratch = GetScratch();
    
    wchar_t *FilePath16 = UTF16FromUTF8(Scratch.Arena, FilePath.Str, (int)FilePath.Length);
    bool Success = (DeleteFileW(FilePath16) != 0);
    
    ReleaseScratch(Scratch);
    
    return Success;
}

void OS_CloseFile(os_file_handle Handle) 
{
    Assert(!"Unimplemented for windows");
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// Memory
//

void *OS_MemoryReserve(u64 Size)
{
    Assert(Size > 0);
    void *Memory = VirtualAlloc(0, Size, MEM_RESERVE, PAGE_NOACCESS);
    return Memory;
}

bool OS_MemoryCommit(void *Address, u64 Size)
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

void OS_MemoryDecommit(void *Address, u64 Size)
{
    // Frees all pages with a byte in the range from Address to Address+Size
    Assert(Address);
    Assert(Size > 0);
    // Does not fail if you try to decommit an uncommited page
    BOOL Success = VirtualFree(Address, Size, MEM_DECOMMIT);
    Assert(Success);
}

void OS_MemoryFree(void *Address, u64)
{
    // Decommits all commited pages and releases reserved memory region
    Assert(Address);
    BOOL Success = VirtualFree(Address, 0, MEM_RELEASE);
    Assert(Success);
}

void OS_MemoryGuard(void *Address, u64 Size)
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

void OS_MemoryRemoveGuard(void *Address, u64 Size)
{
    Assert(Address);
    Assert(Size > 0);
    Assert(((uintptr_t)Address & (4096 - 1)) == 0);
    Assert((Size & (4096 - 1)) == 0);
    
    DWORD OldProtectFlags;
    BOOL Success = VirtualProtect(Address, Size, PAGE_READWRITE, &OldProtectFlags);
    Assert(Success);
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// Time
//

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

u64 OS_GetWallClockMicroseconds() {
    Assert(!"Unimplemented for windows");
    return 0;
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// Windowing
//

HANDLE Win32CreateConsole()
{
    // Attach to existing console
    BOOL ok = AttachConsole((DWORD)-1);
    if (!ok) {
        // Create a new console
        AllocConsole();
    }
    
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONIN", "r", stdin);
    
    HANDLE con_out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD out_mode;
    GetConsoleMode(con_out, &out_mode);
    
    return GetStdHandle(STD_INPUT_HANDLE);
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

os_key_map Win32CreateKeyCodeMap()
{
    os_key_map Map = {};
    
    // The Backspace key
    Map.Keys[VK_BACK] = KeyCode_Backspace;
    Map.Keys[VK_TAB] = KeyCode_Tab;
    Map.Keys[VK_RETURN] = KeyCode_Enter;
    // Caps lock key
    //Map.Keys[VK_CAPITAL] = KeyCode_CapsLock;
    Map.Keys[VK_ESCAPE] = KeyCode_Escape;
    Map.Keys[VK_SPACE] = KeyCode_Space;
    // Page Up key
    Map.Keys[VK_PRIOR] = KeyCode_PageUp;
    // Page Down key
    Map.Keys[VK_NEXT] = KeyCode_PageDown;
    Map.Keys[VK_END] = KeyCode_End;
    Map.Keys[VK_HOME] = KeyCode_Home;
    Map.Keys[VK_INSERT] = KeyCode_Insert;
    Map.Keys[VK_DELETE] = KeyCode_Delete;
    
    //Map.Keys[VK_SHIFT] = KeyCode_Shift;
    //Map.Keys[VK_CONTROL] = KeyCode_Control;
    // Alt key 
    //Map.Keys[VK_MENU] = KeyCode_Alt;
    
    Map.Keys[VK_LEFT] = KeyCode_Left;
    Map.Keys[VK_UP] = KeyCode_Up;
    Map.Keys[VK_RIGHT] = KeyCode_Right;
    Map.Keys[VK_DOWN] = KeyCode_Down;
    
    Map.Keys[(u8)'0'] = KeyCode_0;
    Map.Keys[(u8)'1'] = KeyCode_1;
    Map.Keys[(u8)'2'] = KeyCode_2;
    Map.Keys[(u8)'3'] = KeyCode_3;
    Map.Keys[(u8)'4'] = KeyCode_4;
    Map.Keys[(u8)'5'] = KeyCode_5;
    Map.Keys[(u8)'6'] = KeyCode_6;
    Map.Keys[(u8)'7'] = KeyCode_7;
    Map.Keys[(u8)'8'] = KeyCode_8;
    Map.Keys[(u8)'9'] = KeyCode_9;

    Map.Keys[(u8)'A'] = KeyCode_A;
    Map.Keys[(u8)'B'] = KeyCode_B;
    Map.Keys[(u8)'C'] = KeyCode_C;
    Map.Keys[(u8)'D'] = KeyCode_D;
    Map.Keys[(u8)'E'] = KeyCode_E;
    Map.Keys[(u8)'F'] = KeyCode_F;
    Map.Keys[(u8)'G'] = KeyCode_G;
    Map.Keys[(u8)'H'] = KeyCode_H;
    Map.Keys[(u8)'I'] = KeyCode_I;
    Map.Keys[(u8)'J'] = KeyCode_J;
    Map.Keys[(u8)'K'] = KeyCode_K;
    Map.Keys[(u8)'L'] = KeyCode_L;
    Map.Keys[(u8)'M'] = KeyCode_M;
    Map.Keys[(u8)'N'] = KeyCode_N;
    Map.Keys[(u8)'O'] = KeyCode_O;
    Map.Keys[(u8)'P'] = KeyCode_P;
    Map.Keys[(u8)'Q'] = KeyCode_Q;
    Map.Keys[(u8)'R'] = KeyCode_R;
    Map.Keys[(u8)'S'] = KeyCode_S;
    Map.Keys[(u8)'T'] = KeyCode_T;
    Map.Keys[(u8)'U'] = KeyCode_U;
    Map.Keys[(u8)'V'] = KeyCode_V;
    Map.Keys[(u8)'W'] = KeyCode_W;
    Map.Keys[(u8)'X'] = KeyCode_X;
    Map.Keys[(u8)'Y'] = KeyCode_Y;
    Map.Keys[(u8)'Z'] = KeyCode_Z;
    
    // Left and Right Windows Key
    Map.Keys[VK_LWIN] = KeyCode_Windows;
    Map.Keys[VK_RWIN] = KeyCode_Windows;
    Map.Keys[VK_NUMPAD0] = KeyCode_Numpad0;
    Map.Keys[VK_NUMPAD1] = KeyCode_Numpad1;
    Map.Keys[VK_NUMPAD2] = KeyCode_Numpad2;
    Map.Keys[VK_NUMPAD3] = KeyCode_Numpad3;
    Map.Keys[VK_NUMPAD4] = KeyCode_Numpad4;
    Map.Keys[VK_NUMPAD5] = KeyCode_Numpad5;
    Map.Keys[VK_NUMPAD6] = KeyCode_Numpad6;
    Map.Keys[VK_NUMPAD7] = KeyCode_Numpad7;
    Map.Keys[VK_NUMPAD8] = KeyCode_Numpad8;
    Map.Keys[VK_NUMPAD9] = KeyCode_Numpad9;
    // I'm assuming these are numpad keys
    Map.Keys[VK_MULTIPLY] = KeyCode_NumpadMultiply;
    Map.Keys[VK_ADD] = KeyCode_NumpadAdd;
    // case VK_SEPARATOR:
    Map.Keys[VK_SUBTRACT] = KeyCode_NumpadSubtract;
    Map.Keys[VK_DECIMAL] = KeyCode_NumpadDecimal;
    Map.Keys[VK_DIVIDE] = KeyCode_NumpadDivide;
    
    Map.Keys[VK_F1] = KeyCode_F1;
    Map.Keys[VK_F2] = KeyCode_F2;
    Map.Keys[VK_F3] = KeyCode_F3;
    Map.Keys[VK_F4] = KeyCode_F4;
    Map.Keys[VK_F5] = KeyCode_F5;
    Map.Keys[VK_F6] = KeyCode_F6;
    Map.Keys[VK_F7] = KeyCode_F7;
    Map.Keys[VK_F8] = KeyCode_F8;
    Map.Keys[VK_F9] = KeyCode_F9;
    Map.Keys[VK_F10] = KeyCode_F10;
    Map.Keys[VK_F11] = KeyCode_F11;
    Map.Keys[VK_F12] = KeyCode_F12;
    
    // These may not be sent (only used for GetKeyState())
    //Map.Keys[VK_LSHIFT] = KeyCode_Shift;
    //Map.Keys[VK_RSHIFT] = KeyCode_Shift;
    //Map.Keys[VK_LCONTROL] = KeyCode_Control;
    //Map.Keys[VK_RCONTROL] = KeyCode_Control;
    //Map.Keys[VK_LMENU] = KeyCode_Alt;
    //Map.Keys[VK_RMENU] = KeyCode_Alt;
    
    // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the ';:' key
    Map.Keys[VK_OEM_1] = KeyCode_Semicolon;
    //For any country/region, the '+' key
    Map.Keys[VK_OEM_PLUS] = KeyCode_Plus;
    // For any country/region, the ',' key
    Map.Keys[VK_OEM_COMMA] = KeyCode_Comma;
    // For any country/region, the '-' key
    Map.Keys[VK_OEM_MINUS] = KeyCode_Minus;
    // For any country/region, the '.' key
    Map.Keys[VK_OEM_PERIOD] = KeyCode_Period;
    // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '/?' key
    Map.Keys[VK_OEM_2] = KeyCode_ForwardSlash;
    // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '`~' key
    Map.Keys[VK_OEM_3] = KeyCode_GraveAccent;
    // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '[{' key
    Map.Keys[VK_OEM_4] = KeyCode_LeftSquareBracket;
    // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '\|' key
    Map.Keys[VK_OEM_5] = KeyCode_BackSlash;
    // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the ']}' key
    Map.Keys[VK_OEM_6] = KeyCode_RightSquareBracket;
    // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the 'single-quote/double-quote' key
    Map.Keys[VK_OEM_7] = KeyCode_SingleQuote;
    // Used for miscellaneous characters; it can vary by keyboard.
    // VK_OEM_8:
    // Either the angle bracket key or the backslash key on the RT 102-key keyboard
    // VK_OEM_102:
    
    return Map;
}


LRESULT CALLBACK Win32WindowProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
    LRESULT Result = 0;
    switch (Message)
    {
        // TODO: Clipboard stuff
        
        // TODO: Save to file because computer is turning off
        //case WM_QUERYENDSESSION:
        //case WM_ENDSESSION:
        
        case WM_CLOSE:
        {
            PostQuitMessage(0);
            printf("WindowProc WM_CLOSE\n");
        } break;
        
        case WM_DESTROY:
        {
            printf("WindowProc WM_DESTORY\n");
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
            // Should have been handled by ProcessMessages
            Assert(0);
        }
        
        default:
        {
            Result = DefWindowProcA(Window, Message, wParam, lParam);
        } break;
    }
    
    return Result;
}


void Win32ProcessMessages(HWND Window, os_input *Input, os_key_map *KeyMap)
{
    Input->KeysPressedCount = 0;
    
    Input->Exited = false;
    Input->MouseLeftClick = false;
    Input->MouseRightClick = false;
    Input->MouseLeftDoubleClick = false;
    Input->MouseRightDoubleClick = false;
    Input->MouseScrollY = 0;
    
    MSG Message = {};
    while (true)
    {
        if (PeekMessage(&Message, NULL, NULL, NULL, PM_REMOVE))
        {
            // Got message
        }
        else
        {
            break;
        }
        WPARAM wParam = Message.wParam;
        LPARAM lParam = Message.lParam;
        switch (Message.message)
        {
            case WM_QUIT:
            {
                printf("ProcessMessages WM_QUIT\n");
                Input->Exited = true;
            } break;
            
            case WM_MOUSEWHEEL:
            {
                // In multiples of WHEEL_DELTA (which is 120)
                s32 ScrollDistance = (s16)GET_WHEEL_DELTA_WPARAM(wParam); 
                f32 ScrollY = (f32)ScrollDistance / WHEEL_DELTA;
                Input->MouseScrollY = ScrollY;
            } break;
            
            case WM_MOUSEMOVE:
            {
                s16 X = LOWORD(lParam);
                s16 Y = HIWORD(lParam);
                
                // Multiple monitors can cause negative cursor coordinates
                // I don't know how to handle multiple monitors
                Assert(X >= 0);
                Assert(Y >= 0);
                
                Input->MouseX = (s32)X;
                Input->MouseY = (s32)Y;
                
            } break;
            
            case WM_LBUTTONDOWN: 
            case WM_LBUTTONDBLCLK: 
            case WM_RBUTTONDOWN: 
            case WM_RBUTTONDBLCLK: 
            case WM_LBUTTONUP: 
            case WM_RBUTTONUP: 
            {
                // These are made false at the start of this function
                if (Message.message == WM_LBUTTONDBLCLK)
                {
                    Input->MouseLeftDoubleClick = true;
                }
                if (Message.message == WM_RBUTTONDBLCLK)
                {
                    Input->MouseRightDoubleClick = true;
                }
                
                if (Message.message == WM_LBUTTONDOWN)
                {
                    Input->MouseLeftClick = true;
                    Input->MouseLeftDown = true;
                }
                if (Message.message == WM_RBUTTONDOWN)
                {
                    Input->MouseRightClick = true;
                    Input->MouseRightDown = true;
                }
                
                if (Message.message == WM_LBUTTONUP)
                {
                    Input->MouseLeftDown = false;
                }
                if (Message.message == WM_RBUTTONUP)
                {
                    Input->MouseRightDown = false;
                }
                
            } break;
            
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                // I don't think I need KEYUP messages as the repeat will continue to send KEYDOWN messages
                WPARAM VKCode = wParam;
                if (VKCode == VK_ESCAPE)
                {
                    Input->Exited = true;
                }
                
                bool PreviouslyDown = lParam & (1 << 30);
                bool IsDown = (lParam & (1 << 31)) == 0;
                
                // Many valid keys like modifier keys or extended F-keys we return KeyCode_None from the KeyCodeMap
                if (IsDown)
                {
                    // Key was just pressed
                    os_key_code Key = KeyMap->Keys[VKCode];
                    if (Key != KeyCode_None)
                    {
                        Input->KeysPressedThisFrame[Input->KeysPressedCount] = Key;
                        Input->KeysPressedCount += 1;
                    }
                }
                
                if (PreviouslyDown != IsDown)
                {
                    // State changed from Down -> Up or from Up -> Down
                    os_key_code Key = KeyMap->Keys[VKCode];
                    if (Key != KeyCode_None)
                    {
                        Input->KeyDown[Key] = IsDown;
                    }
                }
            } break;
            
            default:
            {
                TranslateMessage(&Message);
                DispatchMessageA(&Message);
            } break;
        }
    }
    
    // f the high-order bit is 1, the key is down; otherwise, it is up.
    // If the low-order bit is 1, the key is toggled. A key, such as the CAPS LOCK key, is toggled if it is turned
    // on. The key is off and untoggled if the low-order bit is 0. A toggle key's indicator light (if any) on the
    // keyboard will be on when the key is toggled, and off when the key is untoggled.
    SHORT Shift    = GetKeyState(VK_SHIFT);
    SHORT Control  = GetKeyState(VK_CONTROL);
    SHORT Alt      = GetKeyState(VK_MENU);
    SHORT CapsLock = GetKeyState(VK_CAPITAL);
    
    Input->ModifierKeys.ShiftDown       = Shift    & (1 << 15);
    Input->ModifierKeys.ControlDown     = Control  & (1 << 15);
    Input->ModifierKeys.AltDown         = Alt      & (1 << 15);
    Input->ModifierKeys.CapsLockToggled = CapsLock & (1 << 0);
    
    Input->WindowDim = Win32GetWindowDim(Window);
}

void OS_GetEvents(os_window Window, os_input *Input) 
{
    Win32ProcessMessages((HWND)Window.Handle, Input, &GlobalKeyMap);
}

os_window OS_CreateWindow(u8 *WindowName, u32 Width, u32 Height)
{
    HWND Window = 0;
    
    HINSTANCE hInstance = GlobalInstanceHandle;
    
    WNDCLASSA WindowClass = {};
    WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;  
    WindowClass.lpfnWndProc = Win32WindowProc;
    WindowClass.hInstance = hInstance;
    WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    WindowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); 
    WindowClass.lpszClassName = "MaizxcvbnWindowClass";
    if (RegisterClassA(&WindowClass))
    {
        int WindowWidth = Width;
        int WindowHeight = Height;
        
        DWORD WindowStyle = WS_OVERLAPPEDWINDOW;
        // Calculate the size of the window so our client area is what we actually want. 
        // It seems like this may not consistantly work and we may need to different system for fullscreen.
        RECT Rect = {0, 0, WindowWidth, WindowHeight};
        AdjustWindowRect(&Rect, WindowStyle, false);
        
        int RequestedWindowWidth = Rect.right - Rect.left;
        int RequestedWindowHeight = Rect.bottom - Rect.top;
        
        Window = CreateWindowExA(0, 
                                 WindowClass.lpszClassName,
                                 (char *)WindowName,
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
                //ShowWindow(Window, nCmdShow);
                ShowWindow(Window, 1);
            }
        }
    }
    
    os_window Result = { Window };
    return Result;
}

void OS_SwapBuffers(os_window Window) 
{
    HDC WindowDC = GetDC((HWND)Window.Handle);
    SwapBuffers(WindowDC);
}

void OS_DestroyWindow(os_window Window) 
{
    HWND WindowHandle = (HWND)Window.Handle;
    HDC WindowDC = GetDC(WindowHandle);
    ReleaseDC(WindowHandle, WindowDC);
}

#if WIN32_GUI_APP == 1

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)pCmdLine;
    (void)nCmdShow;

    Win32CreateConsole();
    
    GlobalInstanceHandle = hInstance;
    GlobalKeyMap = Win32CreateKeyCodeMap();

    QueryPerformanceFrequency(&GlobalPerformanceFrequency);
    
    int AppMain(int ArgCount, char *Args[]);
    return AppMain(0, NULL);
}

#else

int main(int ArgCount, char *Args[]) {
    QueryPerformanceFrequency(&GlobalPerformanceFrequency);
    
    int AppMain(int ArgCount, char *Args[]);
    return AppMain(0, NULL);
}

#endif