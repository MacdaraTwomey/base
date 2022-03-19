
// Might need to #define GL_GLEXT_PROTOTYPES to get function prototypes
#include "GL/glcorearb.h"

// There is no function prototype for each  OpenGL function, only the global variable defined here.
// function prototype names collide with the global variables.
#define OpenGLFunction(Name, Type) PFNGL##Type##PROC gl##Name = nullptr;
#include "opengl_functions.h"
#undef OpenGLFunction

void OpenGLEndFrame()
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

GLuint CreateShader(GLenum Type, const char *ShaderSource)
{
    GLuint Shader = glCreateShader(Type);
    glShaderSource(Shader, 1, &ShaderSource, nullptr);
    glCompileShader(Shader);
    
    GLint Success;
    glGetShaderiv(Shader, GL_COMPILE_STATUS, &Success);
    if (!Success)
    {
        char InfoLog[2048];
        glGetShaderInfoLog(Shader, sizeof(InfoLog), nullptr, InfoLog);
        printf("ERROR: Could not compile %s shader\n", (Type == GL_VERTEX_SHADER) ? "Vertex" : "Fragment");
        printf(InfoLog);
        Assert(0);
    }
    
    return Shader;
}


GLuint CreateProgram()
{
    const char *VertexShaderSource = R"FOO(
    #version 330 core

uniform mat4x4 Transform;

    in vec2 VertP;
    in vec2 VertUV;
    in vec4 VertColour;
out vec2 FragUV;
out vec4 FragColour;
        
    void main() {
FragUV = VertUV; 
FragColour = VertColour; 
    gl_Position = Transform * vec4(VertP, 0.0, 1.0);
    }
        
    )FOO";
    
    const char *FragmentShaderSource = R"BAR(
    #version 330 core

uniform sampler2D FontTexture;

in vec2 FragUV;
in vec4 FragColour;
    out vec4 OutColour;
        
    void main() {

        //Color = lerp(Background, Foreground, Tex.rgb);

        OutColour = FragColour * texture(FontTexture, FragUV).r;
//OutColour = vec4(FragColour.rgb, texture(FontTexture, FragUV).r);
    }

    )BAR";
    
    GLuint Program = glCreateProgram();
    GLuint VertexShader = CreateShader(GL_VERTEX_SHADER, VertexShaderSource);
    GLuint FragmentShader = CreateShader(GL_FRAGMENT_SHADER, FragmentShaderSource);
    
    glAttachShader(Program, VertexShader);
    glAttachShader(Program, FragmentShader);
    glLinkProgram(Program);
    
    // TODO: Not sure if this is useful
    glValidateProgram(Program);
    
    int Success;
    glGetProgramiv(Program, GL_LINK_STATUS, &Success);
    if (!Success)
    {
        char InfoLog[2048];
        glGetProgramInfoLog(Program, sizeof(InfoLog), nullptr, InfoLog);
        printf("ERROR: Could not link shader program\n");
        printf(InfoLog);
        Assert(0);
    }
    
    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader);
    
    // glGetAttribLocation here
    
    return Program;
}

void OpenGLDebugOutput(GLenum Source, GLenum Type, unsigned int ID, GLenum Severity, 
                       GLsizei, const char *Message, const void *)
{
    if (ID == 131185) 
    {
        return;
    }
    
    char *SourceStr = "No source";
    switch (Source)
    {
        case GL_DEBUG_SOURCE_API:             SourceStr = "Source: API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   SourceStr = "Source: Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: SourceStr = "Source: Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     SourceStr = "Source: Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     SourceStr = "Source: Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           SourceStr = "Source: Other"; break;
    } 
    
    char *TypeStr = "No type";
    switch (Type)
    {
        case GL_DEBUG_TYPE_ERROR:               TypeStr = "Type: Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: TypeStr = "Type: Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  TypeStr = "Type: Undefined Behaviour"; break; 
        case GL_DEBUG_TYPE_PORTABILITY:         TypeStr = "Type: Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         TypeStr = "Type: Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              TypeStr = "Type: Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          TypeStr = "Type: Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           TypeStr = "Type: Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER:               TypeStr = "Type: Other"; break;
    } 
    
    char *SeverityStr = "No severity";
    switch (Severity)
    {
        
        case GL_DEBUG_SEVERITY_HIGH:         SeverityStr = "Severity: high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       SeverityStr = "Severity: medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          SeverityStr = "Severity: low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: SeverityStr = "Severity: notification"; break;
    } 
    
    printf("---------------\n");
    printf("Debug message (%u): %s\n", ID, Message);
    printf("%s\n", SourceStr);
    printf("%s\n", TypeStr);
    printf("%s\n", SeverityStr);
    printf("---------------\n");
}


void InitOpenGL()
{
    GLint Flags; 
    glGetIntegerv(GL_CONTEXT_FLAGS, &Flags);
    if (Flags & GL_CONTEXT_FLAG_DEBUG_BIT)
    {
        GLint Major = 0;
        GLint Minor = 0;
        glGetIntegerv(GL_MAJOR_VERSION, &Major);
        glGetIntegerv(GL_MINOR_VERSION, &Minor);
        if (Major > 4 || (Major >= 4 && Minor >= 3))
        {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(OpenGLDebugOutput, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        }
    }
    
    GLuint Program = CreateProgram();
    
}