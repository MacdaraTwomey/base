
// Might can #define GL_GLEXT_PROTOTYPES to get function prototypes
#include "GL/glcorearb.h"

struct vertex
{
    v2 P;
    v2 UV;
    u32 Colour;
};

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
        printf("%s", InfoLog);
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

uniform sampler2D Texture;

in vec2 FragUV;
in vec4 FragColour;
    out vec4 OutColour;
        
    void main() {
 OutColour = FragColour * texture(Texture, FragUV); 
    }

    )BAR";
    
    GLuint Program = glCreateProgram();
    GLuint VertexShader = CreateShader(GL_VERTEX_SHADER, VertexShaderSource);
    GLuint FragmentShader = CreateShader(GL_FRAGMENT_SHADER, FragmentShaderSource);
    
    glAttachShader(Program, VertexShader);
    glAttachShader(Program, FragmentShader);
    glLinkProgram(Program);
    
    // This information may already appear in glDebugMessageCallback
    glValidateProgram(Program);
    
    int Success;
    glGetProgramiv(Program, GL_LINK_STATUS, &Success);
    if (!Success)
    {
        char InfoLog[2048];
        glGetProgramInfoLog(Program, sizeof(InfoLog), nullptr, InfoLog);
        printf("ERROR: Could not link shader program\n");
        printf("%s", InfoLog);
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
        
        // NOTE: We can call this even with a OpenGL 3.2 context on my machine (which is convinient) 
        // but this can't be relied on.
        
        //if (Major > 4 || (Major >= 4 && Minor >= 3))
        {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(OpenGLDebugOutput, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        }
    }
    
    GLuint Program = CreateProgram();
    glUseProgram(Program);
    
    GLuint VAO = 0;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    GLuint VBO = 0;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    
    GLint VertexPositionIndex = glGetAttribLocation(Program, "VertP");
    glEnableVertexAttribArray(VertexPositionIndex);
    glVertexAttribPointer(VertexPositionIndex, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, P));
    
    s32 VertexUVIndex = glGetAttribLocation(Program, "VertUV");
    glEnableVertexAttribArray(VertexUVIndex);
    glVertexAttribPointer(VertexUVIndex, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, UV));
    
    // We are giving shader a u32 which is 4 unsigned bytes, each of which will be normalised to [0.0-1.0]
    GLint VertexColourIndex = glGetAttribLocation(Program, "VertColour");
    glEnableVertexAttribArray(VertexColourIndex);
    glVertexAttribPointer(VertexColourIndex, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(vertex), (void *)offsetof(vertex, Colour));
    
    s32 TransformUniformLocation = glGetUniformLocation(Program, "Transform");
    
    v3 ClearColour = V3(0.5f, 0.5f, 0.5f);
    glClearColor(ClearColour.r, ClearColour.g, ClearColour.b, 1.0f);
    //glEnable(GL_FRAMEBUFFER_SRGB); 
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
}

void OpenGLBeginFrame()
{
    
}

void OpenGLEndFrame(s32 WindowWidth, s32 WindowHeight)
{
    
}
