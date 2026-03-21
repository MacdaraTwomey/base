#pragma once

#include "base.h"


///////////////////////////////////////////////////////////////////////
// Filesystem
//

// TODO: File handle API, with error conditions available via a call to OS_GetFileError or something.
// TODO: Handle deleteing directories

typedef s32 os_file_handle;

enum os_file_info {
    OS_FileInfo_Error,
    OS_FileInfo_File,
    OS_FileInfo_Directory,
};

struct os_directory_iterator {
    void *Handle;
};

struct os_filesystem_entry {
    string Name;
    os_file_info Info;
};

bool                    OS_FileExists(string FilePath);
os_file_info            OS_GetFileInfo(string Path);
u64                     OS_GetFileSize(string FilePath);
string                  OS_GetExecutablePath(arena *Arena);
string                  OS_CanonicalAbsolutePath(arena *Arena, string Path);

os_directory_iterator   OS_DirectoryIterator(string DirectoryPath);
void                    OS_DirectoryIteratorClose(os_directory_iterator *Iter);
bool                    OS_DirectoryIteratorIsOk(os_directory_iterator Iter);
os_filesystem_entry     OS_DirectoryIteratorNext(arena *Arena, os_directory_iterator *Iter);

string                  OS_ReadEntireFile(arena *Arena, string FilePath);
bool                    OS_WriteEntireFile(u64 Size, u8 *Contents, string FilePath);

bool                    OS_DeleteFile(string FilePath);
void                    OS_CloseFile(os_file_handle Handle);


///////////////////////////////////////////////////////////////////////
// Memory
//

void *  OS_MemoryReserve(u64 Size);
bool    OS_MemoryCommit(void *Address, u64 size);
void    OS_MemoryDecommit(void *Address, u64 Size);
// Size must match the original reserved Size. 
// On Windows Size is not used, the entire allocation is freed. 
// On Linux Size is used to determine how many pages to free.
void    OS_MemoryFree(void *Address, u64 Size);
void    OS_MemoryGuard(void *Address, u64 Size);
void    OS_MemoryRemoveGuard(void *Address, u64 Size);


///////////////////////////////////////////////////////////////////////
// Time

u64 OS_GetWallClockMicroseconds();

///////////////////////////////////////////////////////////////////////
// Windowing


enum os_key_code : u8
{
    // NOTE: We rely on the relative order of these key codes
    KeyCode_None = 0,
    
    KeyCode_A,
    KeyCode_B,
    KeyCode_C,
    KeyCode_D,
    KeyCode_E,
    KeyCode_F,
    KeyCode_G,
    KeyCode_H,
    KeyCode_I,
    KeyCode_J,
    KeyCode_K,
    KeyCode_L,
    KeyCode_M,
    KeyCode_N,
    KeyCode_O,
    KeyCode_P,
    KeyCode_Q,
    KeyCode_R,
    KeyCode_S,
    KeyCode_T,
    KeyCode_U,
    KeyCode_V,
    KeyCode_W,
    KeyCode_X,
    KeyCode_Y,
    KeyCode_Z,
    
    KeyCode_0,
    KeyCode_1,
    KeyCode_2,
    KeyCode_3,
    KeyCode_4,
    KeyCode_5,
    KeyCode_6,
    KeyCode_7,
    KeyCode_8,
    KeyCode_9,
    
    KeyCode_F1,
    KeyCode_F2,
    KeyCode_F3,
    KeyCode_F4,
    KeyCode_F5,
    KeyCode_F6,
    KeyCode_F7,
    KeyCode_F8,
    KeyCode_F9,
    KeyCode_F10,
    KeyCode_F11,
    KeyCode_F12,
    
    KeyCode_Tab,
    KeyCode_Windows,
    KeyCode_Escape,
    KeyCode_Space,
    KeyCode_Backspace,
    KeyCode_Enter,
    KeyCode_Delete,
    KeyCode_Insert,
    
    KeyCode_Left,
    KeyCode_Right,
    KeyCode_Up,
    KeyCode_Down,
    
    KeyCode_PageUp,
    KeyCode_PageDown,
    KeyCode_End,
    KeyCode_Home,
    
    KeyCode_Minus,
    KeyCode_Plus,
    KeyCode_GraveAccent,
    KeyCode_LeftSquareBracket,
    KeyCode_RightSquareBracket,
    KeyCode_BackSlash,
    KeyCode_Semicolon,
    KeyCode_SingleQuote,
    KeyCode_Period,
    KeyCode_Comma,
    KeyCode_ForwardSlash,
    
    // For numpad keys I'm assuming these are all numpad keys 
    KeyCode_Numpad0,
    KeyCode_Numpad1,
    KeyCode_Numpad2,
    KeyCode_Numpad3,
    KeyCode_Numpad4,
    KeyCode_Numpad5,
    KeyCode_Numpad6,
    KeyCode_Numpad7,
    KeyCode_Numpad8,
    KeyCode_Numpad9,
    KeyCode_NumpadMultiply,
    KeyCode_NumpadAdd,
    KeyCode_NumpadDecimal,
    KeyCode_NumpadDivide,
    KeyCode_NumpadSubtract,
    
    KeyCode_COUNT,
};

struct os_key_map 
{
    os_key_code Keys[0xFF];
};


//
//enum mouse_button
//{
//MouseButton_Left,
//MouseButton_Right,
//MouseButton_Middle,
//};
//
struct os_modifier_keys
{
    bool ShiftDown;
    bool ControlDown;
    bool AltDown;
    bool CapsLockToggled;
};

#define MOUSE_BUTTON_COUNT 3

struct os_input
{
    bool Exited;
    
    v2s WindowDim;
    
    // We have redundant input information so I can see which is the most useful to keep
    os_key_code KeysPressedThisFrame[100];
    u32 KeysPressedCount;
    
    u8 KeyDown[KeyCode_COUNT];
    
    os_modifier_keys ModifierKeys;
    
    bool MouseLeftClick;
    bool MouseRightClick;
    bool MouseLeftDoubleClick;
    bool MouseRightDoubleClick;
    bool MouseLeftDown;
    bool MouseRightDown;
    
    // Negative is backwards and down towards the user, positive is forwards and away from the user
    f32 MouseScrollY;
    
    // In pixels with Y increasing down the screen
    s32 MouseX;
    s32 MouseY;
};

struct os_window {
    void *Handle;
};

os_window OS_CreateWindow(u8 *WindowName, u32 Width, u32 Height);
void OS_DestroyWindow(os_window Window);
void OS_GetEvents(os_window Window, os_input *Input);
void OS_SwapBuffers(os_window Window);