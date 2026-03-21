
#include <windows.h>

//#define GL_VERSION_3_3 0
//#define GL_VERSION_4_0 0
//#define GL_VERSION_4_1 0
//#define GL_VERSION_4_2 0
//#define GL_VERSION_4_3 0
//#define GL_VERSION_4_4 0
//#define GL_VERSION_4_5 0
//#define GL_VERSION_4_6 0
#include "GL/glcorearb.h"
#include "GL/wglext.h"  

// There is no function prototype for each  OpenGL function, only the global variable defined here.
// function prototype names collide with the global variables.
#define OpenGLFunction(Name, Type) PFNGL##Type##PROC gl##Name = nullptr;
#include "opengl_functions.h"
#undef OpenGLFunction

void *Win32LoadOpenGLFunction(HMODULE OpenGL32DLL, const char *Name)
{
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

void Win32LoadOpenGLFunctions()
{
    HMODULE OpenGL32DLL = LoadLibraryA("opengl32.dll"); 
    if (OpenGL32DLL)
    {
#define OpenGLFunction(Name, Type) gl##Name = (PFNGL##Type##PROC) Win32LoadOpenGLFunction(OpenGL32DLL, "gl"#Name);
#include "opengl_functions.h"
#undef OpenGLFunction
    }
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
    
    // Reqire sRGB support
    if (sRGBSupported)
    {
        int AttributeList[] = {
            WGL_DRAW_TO_WINDOW_ARB,  GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB,  GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB,   GL_TRUE,
            WGL_PIXEL_TYPE_ARB,      WGL_TYPE_RGBA_ARB,
            WGL_ACCELERATION_ARB,    WGL_FULL_ACCELERATION_ARB,
            WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, ((sRGBSupported) ? GL_TRUE : GL_FALSE),
            WGL_COLOR_BITS_ARB,      24,  // DescribePixelFormat turns this to 32, but docs say it should be 24...
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
    }
    
    return Success;
}

// If we can't create a modern context then we don't try to use a older context.
// NOTE: sRGB framebuffers have been core since OpenGL 3.0. We should just error
// if they are not supported, when write one code path.
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
                                        if (StringMatch(Extension, Strlit("WGL_EXT_framebuffer_sRGB")) ||
                                            StringMatch(Extension, Strlit("WGL_ARB_framebuffer_sRGB")))
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
                                // This may return a higher version that was requested, but it will never return
                                // a version that does not implement the core features that your version implements.
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

bool Win32InitOpenGL(HDC WindowDC)
{
    bool Success = false;
    if (Win32CreateOpenGLContext(WindowDC))
    {
        SetSwapInterval(1);
        Win32LoadOpenGLFunctions();
        Success = true;
    }
    
    return Success;
}
