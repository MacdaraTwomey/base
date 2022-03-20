
// Might need to #define GL_GLEXT_PROTOTYPES to get function prototypes
#include "GL/glcorearb.h"

// There is no function prototype for each  OpenGL function, only the global variable defined here.
// function prototype names collide with the global variables.
#define OpenGLFunction(Name, Type) PFNGL##Type##PROC gl##Name = nullptr;
#include "opengl_functions.h"
#undef OpenGLFunction

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
    in vec4 VertColour;
out vec4 FragColour;
        
    void main() {
FragColour = VertColour; 
    gl_Position = Transform * vec4(VertP, 0.0, 1.0);
    }
        
    )FOO";
    
    const char *FragmentShaderSource = R"BAR(
    #version 330 core

in vec4 FragColour;
    out vec4 OutColour;
        
    void main() {
 OutColour = FragColour;
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

struct vertex
{
    v2 P;
    u32 Colour;
};

GLuint GlobalVAO = 0;
GLuint GlobalVBO = 0;
GLint GlobalTransformUniform = 0;

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
    
    // We are giving shader a u32 which is 4 unsigned bytes, each of which will be normalised to [0.0-1.0]
    GLint VertexColourIndex = glGetAttribLocation(Program, "VertColour");
    glEnableVertexAttribArray(VertexColourIndex);
    glVertexAttribPointer(VertexColourIndex, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(vertex), (void *)offsetof(vertex, Colour));
    
    // Is there a benefit to matching attrib position in buffer with attrib layout in shader
    printf("VertP Index = %i\n", VertexPositionIndex);
    printf("VertColour Index = %i\n", VertexColourIndex);
    
    s32 TransformUniformLocation = glGetUniformLocation(Program, "Transform");
    
    v3 ClearColour = V3(1.0f, 0.0f, 1.0f);
    glClearColor(ClearColour.r, ClearColour.g, ClearColour.b, 1.0f);
    //glEnable(GL_FRAMEBUFFER_SRGB); // output of fragment shader is gamma corrected pow(1/2.2)
    
    GlobalVAO = VAO;
    GlobalVBO = VBO;
    GlobalTransformUniform = TransformUniformLocation;
}


// TODO: REMOVE
inline u32
RGBAPack4x8(v4 Unpacked)
{
    u32 result = (((u32)roundf(Unpacked.a) << 24) |
                  ((u32)roundf(Unpacked.b) << 16) |
                  ((u32)roundf(Unpacked.g) << 8) |
                  ((u32)roundf(Unpacked.r) << 0));
    
    return result;
}

void OpenGLBeginFrame()
{
    
}

void OpenGLEndFrame(s32 WindowWidth, s32 WindowHeight)
{
    
    vertex Vert0 = vertex{V2(0, 0), RGBAPack4x8(V4(1.0f, 0.0f, 0.0f, 1.0f) * 255.0f)};
    vertex Vert1 = vertex{V2((f32)WindowWidth, 0), RGBAPack4x8(V4(0.0f, 1.0f, 0.0f, 1.0f) * 255.0f)};
    vertex Vert2 = vertex{V2((f32)WindowWidth, (f32)WindowHeight), RGBAPack4x8(V4(0.0f, 0.0f, 1.0f, 1.0f) * 255.0f)};
    vertex Vert3 = vertex{V2(0, (f32)WindowHeight), RGBAPack4x8(V4(8.0f, 3.0f, 4.0f, 1.0f) * 255.0f)};
    
    f32 Dim = 100;
    f32 CentreX = WindowWidth / 2.0f;
    f32 CentreY = WindowHeight / 2.0f;
    
    vertex VertA0 = vertex{V2(CentreX - Dim, CentreY - Dim), RGBAPack4x8(V4(0.0f, 0.0f, 0.0f, 1.0f) * 255.0f)};
    vertex VertA1 = vertex{V2(CentreX + Dim, CentreY - Dim), RGBAPack4x8(V4(0.0f, 0.0f, 0.0f, 1.0f) * 255.0f)};
    vertex VertA2 = vertex{V2(CentreX + Dim, CentreY + Dim), RGBAPack4x8(V4(0.0f, 0.0f, 0.0f, 1.0f) * 255.0f)};
    vertex VertA3 = vertex{V2(CentreX - Dim, CentreY + Dim), RGBAPack4x8(V4(0.0f, 0.0f, 0.0f, 1.0f) * 255.0f)};
    
    vertex VertexArray[] = {
        Vert0, Vert2, Vert3,
        Vert0, Vert1, Vert2,
        
        VertA0, VertA2, VertA3,
        VertA0, VertA1, VertA2,
    };
    
    glBindBuffer(GL_ARRAY_BUFFER, GlobalVBO);
    glBufferData(GL_ARRAY_BUFFER, ArrayCount(VertexArray) * sizeof(vertex), VertexArray, GL_DYNAMIC_DRAW);
    
    Assert(WindowWidth > 0 && WindowHeight > 0);
    m4x4 VertexTransform = OrthographicProjection2D(0, (f32)WindowWidth, 0, (f32)WindowHeight);
    
    glUniformMatrix4fv(GlobalTransformUniform, 1, GL_TRUE, &VertexTransform.E[0][0]);
    
    glViewport(0, 0, WindowWidth, WindowHeight);
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)ArrayCount(VertexArray));
}
