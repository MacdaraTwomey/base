
#include "graphics.h"
#include "base.h"

#include <cstddef> // offsetof
#include <stdio.h> //fopen for WriteImage

// Can #define GL_GLEXT_PROTOTYPES to get function prototypes
// but i think we don't want this as we will define them as function pointers in the OpenGL loader.
#include "GL/glcorearb.h"

//
// TODO:
// * Change index to use u16 and use a base vertex to compensate 
// * remove Size from render_entry_header, instead make each type OpenGLEndFrame advance the pointer
//   by that types size.
//

u8 *PushRenderEntry(render_commands *RenderCommands, render_entry_type Type)
{
    u8 *Entry = nullptr;
    
    if (Type != RenderEntryType_TexturedQuads)
    {
        RenderCommands->LastTexturedQuadEntry = nullptr;
    }
    
    u32 Size = 0;
    switch (Type)
    {
        case RenderEntryType_Clear: Size = sizeof(render_entry_clear); break;
        case RenderEntryType_Clip:  Size = sizeof(render_entry_clip); break;
        case RenderEntryType_Line:  Size = sizeof(render_entry_line); break;
        case RenderEntryType_TexturedQuads: Size = sizeof(render_entry_textured_quads); break;
        default: Assert(0);
    }
    
    u32 Remaining = RenderCommands->PushBufferSize - (u32)(ptrdiff_t)(RenderCommands->PushBufferAt - RenderCommands->PushBufferBase);
    if (sizeof(render_entry_header) + Size <= Remaining)
    {
        render_entry_header *Header = (render_entry_header *)RenderCommands->PushBufferAt;
        Header->Type = Type;
        
        RenderCommands->PushBufferAt += sizeof(render_entry_header);
        
        Entry = RenderCommands->PushBufferAt;
        
        RenderCommands->PushBufferAt += Size;
    }
    
    return Entry;
}

#if 0
void PushLine(render_commands *RenderCommands,
              u32 Colour, f32 LineWidth, 
              v2 FromP, v2 ToP)
{
    Assert(RenderCommands->VertexCount + 2 <= RenderCommands->MaxVertexCount);
    Assert(RenderCommands->IndexCount + 2 <= RenderCommands->MaxIndexCount);
    
    u32 VertexIndex = RenderCommands->VertexCount;
    vertex *Vertex = RenderCommands->VertexArray + VertexIndex;
    
    
    Vertex[0].P = FromP;
    Vertex[0].Colour = Colour;
    Vertex[1].P = ToP;
    Vertex[1].Colour = Colour;
    
    u32 IndexOffset = RenderCommands->IndexCount;
    u32 *Index = RenderCommands->IndexArray + IndexOffset;
    
    Index[0] = VertexIndex + 0;
    Index[1] = VertexIndex + 1;
    
    RenderCommands->VertexCount += 2;
    RenderCommands->IndexCount += 2;
    
    render_entry_line *Entry = (render_entry_line *)PushRenderEntry(RenderCommands, RenderEntryType_Line);
    Entry->IndexOffset = IndexOffset;
    Entry->LineWidth = LineWidth;
}
#endif

void PushQuad(render_commands *RenderCommands,
              u32 TextureHandle,
              v2 P0, v2 UV0, u32 C0,
              v2 P1, v2 UV1, u32 C1,
              v2 P2, v2 UV2, u32 C2, 
              v2 P3, v2 UV3, u32 C3)
{
    Assert(RenderCommands->VertexCount + 4 <= RenderCommands->MaxVertexCount);
    Assert(RenderCommands->IndexCount + 6 <= RenderCommands->MaxIndexCount);
    
    u32 VertexIndex = RenderCommands->VertexCount;
    vertex *Vertex = RenderCommands->VertexArray + VertexIndex;
    
    // 0--3
    // |  |
    // 1--2
    Vertex[0].P = P0;
    Vertex[0].UV = UV0;
    Vertex[0].Colour = C0;
    Vertex[1].P = P1;
    Vertex[1].UV = UV1;
    Vertex[1].Colour = C1;
    Vertex[2].P = P2;
    Vertex[2].UV = UV2;
    Vertex[2].Colour = C2;
    Vertex[3].P = P3;
    Vertex[3].UV = UV3;
    Vertex[3].Colour = C3;
    
    
    // In OpenGL the default triangle winding order used to determine the front face in face culling is Counter-Clockwise
    u32 IndexOffset = RenderCommands->IndexCount;
    u32 *Index = RenderCommands->IndexArray + IndexOffset;
    
    // 0 -> 1 -> 2
    Index[0] = VertexIndex + 0;
    Index[1] = VertexIndex + 1;
    Index[2] = VertexIndex + 2;
    
    // 0 -> 2 -> 3
    Index[3] = VertexIndex + 0;
    Index[4] = VertexIndex + 2;
    Index[5] = VertexIndex + 3;
    
    RenderCommands->VertexCount += 4;
    RenderCommands->IndexCount += 6;
    
    render_entry_textured_quads *LastEntry = RenderCommands->LastTexturedQuadEntry;
    if (LastEntry && 
        (LastEntry->TextureHandle == TextureHandle) && 
        (((LastEntry->QuadCount + 4) * 6) <= UINT16_MAX))
    {
        // Reuse last entry if  using the same texture and can still make indices using a u16
        LastEntry->QuadCount += 1;
        // Keep index offset the same as indices are contiguous
    }
    else
    {
        render_entry_textured_quads *Entry =
        (render_entry_textured_quads *)PushRenderEntry(RenderCommands, RenderEntryType_TexturedQuads);
        if (Entry)
        {
            Entry->IndexOffset = IndexOffset;
            Entry->QuadCount = 1;
            Entry->TextureHandle = TextureHandle;
            
            RenderCommands->LastTexturedQuadEntry = Entry;
        }
    }
}

inline u32
RGBAPack4x8(v4 Unpacked)
{
    u32 result = ((RoundToU32(Unpacked.a) << 24) |
                  (RoundToU32(Unpacked.b) << 16) |
                  (RoundToU32(Unpacked.g) << 8) |
                  (RoundToU32(Unpacked.r) << 0));
    
    return result;
}

inline u32
BGRAPack4x8(v4 Unpacked)
{
    u32 result = ((RoundToU32(Unpacked.a) << 24) |
                  (RoundToU32(Unpacked.r) << 16) |
                  (RoundToU32(Unpacked.g) << 8) |
                  (RoundToU32(Unpacked.b) << 0));
    
    return result;
}

void PushQuad(render_commands *RenderCommands,
              u32 TextureHandle, v4 Colour,
              v2 P0, v2 UV0,
              v2 P1, v2 UV1,
              v2 P2, v2 UV2, 
              v2 P3, v2 UV3)
{
    PushQuad(RenderCommands, 
             TextureHandle,
             P0, UV0, RGBAPack4x8(Colour * 255.0f),
             P1, UV1, RGBAPack4x8(Colour * 255.0f),
             P2, UV2, RGBAPack4x8(Colour * 255.0f),
             P3, UV3, RGBAPack4x8(Colour * 255.0f));
}

void PushColouredQuad(render_commands *RenderCommands,
                      v4 Colour,
                      v2 P0,
                      v2 P1,
                      v2 P2, 
                      v2 P3)
{
    v2 UV = V2(0, 0);
    PushQuad(RenderCommands, 
             RenderCommands->WhiteTextureHandle,
             P0, UV, RGBAPack4x8(Colour * 255.0f),
             P1, UV, RGBAPack4x8(Colour * 255.0f),
             P2, UV, RGBAPack4x8(Colour * 255.0f),
             P3, UV, RGBAPack4x8(Colour * 255.0f));
}

// NOTE: This only works for grid fitted lines
void PushLine(render_commands *RenderCommands,
              u32 Colour, f32 LineWidth, 
              v2 FromP, v2 ToP)
{
    
    // |----------------------------|
    // o -------------------------- o
    // |----------------------------|
    v2 UV = V2(0, 0);
    v2 Side = (0.5f * LineWidth) * Normalize(Perp(ToP - FromP));
    PushQuad(RenderCommands, 
             RenderCommands->WhiteTextureHandle,
             FromP - Side, UV, Colour,
             ToP - Side, UV, Colour,
             ToP + Side,   UV, Colour,
             FromP + Side,   UV, Colour);
}

void PushQuadOutline(render_commands *RenderCommands,
                     v4 Colour, f32 LineWidth,
                     v2 P0,
                     v2 P1,
                     v2 P2, 
                     v2 P3)
{
    // 0--3
    // |  |
    // 1--2
    
    u32 PackedColour = RGBAPack4x8(Colour * 255.0f);
    
    // TODO: Make it so these are batched as one draw call rather than 4 separate lines with 4 draw calls
    PushLine(RenderCommands, PackedColour, LineWidth, P0, P1);
    PushLine(RenderCommands, PackedColour, LineWidth, P1, P2);
    PushLine(RenderCommands, PackedColour, LineWidth, P2, P3);
    PushLine(RenderCommands, PackedColour, LineWidth, P3, P0);
}

void PushClear(render_commands *RenderCommands, v4 ClearColour)
{
    render_entry_clear *Entry = (render_entry_clear *)PushRenderEntry(RenderCommands, RenderEntryType_Clear);
    if (Entry)
    {
        Entry->ClearColour = ClearColour;
    }
}

void PushClipRect(render_commands *RenderCommands, s32 x, s32 y, s32 Width, s32 Height)
{
    render_entry_clip *Entry = (render_entry_clip *)PushRenderEntry(RenderCommands, RenderEntryType_Clip);
    if (Entry)
    {
        Entry->x = x;
        Entry->y = y;
        Entry->Width = Width;
        Entry->Height = Height;
    }
}


m4x4 OrthographicProjection(f32 Left, f32 Right, f32 Bottom, f32 Top, f32 Near, f32 Far)
{
    // Map the left and right screen coordinates to -1 to 1 (For OpenGL)
    // l <= x <= r            If x is a point within left and right:
    // 0 <= x-l <= r-l    Make the left side 0 by subtracting l from all terms
    // 0 <= x-l / (r-l) <= 1   Make the right side 1 by dividing by (r - l)
    // 0 <= 2((x-l) / (r-l)) <= 2 
    // -1 <= 2((x-l) / (r-l)) - 1 <= 1 Multiply by 2 and subtract to to map to [-1, 1]
    
    // -1 <= 2((x-l) / (r-l)) - ((r-l)/(r-l)) <= 1   Replace -1 with ((r-l)/(r-l))
    // -1 <= (2x-l-r) / (r-l) <= 1   Simplify
    // -1 <= 2x / (r-l) - (r+l) / (r-l) <= 1   Gives us the formula to transform x
    // x' = (2 / (r-l)) * x - (r+l) / (r-l)
    
    // Then by using this in the rows of our matrix 
    // vec = (x, 0, 0, 1) dot matrix x row = (2/(r-l), 0, 0, -(r+l)/(r-l))  
    //     = x'
    // Giving us the x component for the projected vector.
    // This works similarly for y and z components (z is negated as the camera faces the -z axis)
    
    // Maps a rectangular volume to a cube centred at (0, 0, 0) with with corners having coordinates -1 to 1
    // The resulting coordinates are NDC which are from -1 to 1 for each component.
    
    f32 a = 2.0f / (Right - Left);
    f32 b = 2.0f / (Top - Bottom);
    f32 c = -2.0f / (Far - Near); 
    
    f32 d = -(Right + Left) / (Right - Left);
    f32 e = -(Top + Bottom) / (Top - Bottom);
    f32 f = -(Far + Near) / (Far - Near);
    
    m4x4 Result = {
        a,  0,  0,  d,
        0,  b,  0,  e,
        0,  0,  c,  f,
        0,  0,  0,  1,
    };
    
    return Result;
}

m4x4 OrthographicProjection(f32 Left, f32 Right, f32 Bottom, f32 Top)
{
    f32 a = 2.0f / (Right - Left);
    f32 b = 2.0f / (Top - Bottom);
    
    f32 d = -(Right + Left) / (Right - Left);
    f32 e = -(Top + Bottom) / (Top - Bottom);
    
    m4x4 Result = {
        a,  0,  0,  d,
        0,  b,  0,  e,
        0,  0,  -1, 0,
        0,  0,  0,  1, 
    };
    
    return Result;
}

v3 Orbit(v3 CameraP, v3 Target, f32 AzimuthRadians, f32 InclinationRadians)
{
    // Transform to spherical coordinates, add azimuth and inclination, then transform back.
    // Assumes z-is-up right handed coordinate system.
    
    v3 P = CameraP - Target;
    
    f32 r = Length(P);
    P = NOZ(P);
    
    f32 Inclination = ACos(P.z) + InclinationRadians;
    f32 Azimuth = ATan2(P.y, P.x) + AzimuthRadians;
    
    Inclination = Clamp(Inclination, 0.01f, Pi32 - 0.01f);
    
    f32 x = Cos(Azimuth) * Sin(Inclination);
    f32 y = Sin(Azimuth) * Sin(Inclination);
    f32 z = Cos(Inclination);
    v3 RotatedP = r * V3(x, y, z);
    
    v3 Result = RotatedP + Target;
    
    return Result;
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// OpenGL specific stuff
//


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
        return 0;
    }
    
    return Shader;
}


GLuint CreateShaderProgram()
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
        return 0;
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


u32 OpenGL_CreateTexture(u32 Width, u32 Height, u8 *Texels)
{
    u32 TextureHandle = 0;
    
    glGenTextures(1, &TextureHandle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TextureHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
    
    // GL_RED:
    // Each element is a single red component. The GL converts it to floating point and assembles it into an RGBA element by attaching 0 for green and blue, and 1 for alpha. Each component is clamped to the range [0,1]. 
    // Maybe should be max 1024x1024? because this is the largest guaranteed supported texture size
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, Width, Height, 0, GL_RED,  GL_UNSIGNED_BYTE, Texels);
    
    Assert(TextureHandle != 0);
    
    return TextureHandle;
}

void OpenGL_DeleteTexture(u32 TextureHandle)
{
    glDeleteTextures(1, &TextureHandle);
}

void OpenGL_Init(opengl *OpenGL)
{
    int Flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &Flags);
    if (Flags & GL_CONTEXT_FLAG_DEBUG_BIT)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(OpenGLDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }
    
    u32 Program = CreateShaderProgram();
    glUseProgram(Program);
    
    u32 VAO = 0;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    u32 VBO = 0;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    
    u32 EBO = 0;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    
    s32 VertexPositionIndex = glGetAttribLocation(Program, "VertP");
    glEnableVertexAttribArray(VertexPositionIndex);
    glVertexAttribPointer(VertexPositionIndex, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, P));
    
    s32 VertexUVIndex = glGetAttribLocation(Program, "VertUV");
    glEnableVertexAttribArray(VertexUVIndex);
    glVertexAttribPointer(VertexUVIndex, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, UV));
    
    // We are giving shader a u32 which is 4 unsigned bytes, each of which will be normalised to [0.0-1.0]
    s32 VertexColourIndex = glGetAttribLocation(Program, "VertColour");
    glEnableVertexAttribArray(VertexColourIndex);
    glVertexAttribPointer(VertexColourIndex, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(vertex), (void *)offsetof(vertex, Colour));
    
    s32 TransformUniformLocation = glGetUniformLocation(Program, "Transform");
    
    u8 FlatTextureData[] = {0xFF, 0xFF, 0xFF, 0xFF};
    u32 WhiteTexture = OpenGL_CreateTexture(2, 2, FlatTextureData);
    
    //glEnable(GL_FRAMEBUFFER_SRGB);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
    
    glEnable(GL_SCISSOR_TEST);
    //glEnable(GL_MULTISAMPLE);
    
    OpenGL->Program = Program;
    OpenGL->VAO = VAO;
    OpenGL->VBO = VBO;
    OpenGL->EBO = EBO;
    OpenGL->TransformUniformLocation = TransformUniformLocation;
    OpenGL->WhiteTexture = WhiteTexture;
}

render_commands *OpenGL_BeginFrame(opengl *OpenGL)
{
    render_commands *Commands = &OpenGL->RenderCommands;
    
    *Commands = {};
    
    Commands->VertexArray = OpenGL->VertexArray;
    Commands->VertexCount = 0;
    Commands->MaxVertexCount = (u32)ArrayCount(OpenGL->VertexArray);
    
    Commands->IndexArray = OpenGL->IndexArray;
    Commands->IndexCount = 0;
    Commands->MaxIndexCount = (u32)ArrayCount(OpenGL->IndexArray);
    
    Commands->PushBufferBase = OpenGL->PushBufferMemory;
    Commands->PushBufferAt = Commands->PushBufferBase;
    Commands->PushBufferSize = sizeof(OpenGL->PushBufferMemory);
    
    MemoryZero(Commands->MaxVertexCount * sizeof(vertex), Commands->VertexArray);
    MemoryZero(Commands->MaxIndexCount * sizeof(u32), Commands->IndexArray);
    MemoryZero(Commands->PushBufferSize, Commands->PushBufferBase);
    
    Commands->WhiteTextureHandle = OpenGL->WhiteTexture;
    Commands->LastTexturedQuadEntry = nullptr;
    
    return Commands;
}

void OpenGL_EndFrame(opengl *OpenGL, render_commands *RenderCommands, 
                     u32 WindowWidth, u32 WindowHeight)
{
    if (WindowWidth == 0 || WindowHeight == 0)
    {
        return;
    }
    
    glBindVertexArray(OpenGL->VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, OpenGL->VBO);
    glBufferData(GL_ARRAY_BUFFER, RenderCommands->VertexCount * sizeof(vertex), RenderCommands->VertexArray, GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGL->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, RenderCommands->IndexCount * sizeof(u32), RenderCommands->IndexArray, GL_DYNAMIC_DRAW);
    
    // TODO: Only need to recompute on size change
    Assert(WindowWidth > 0 && WindowHeight > 0);
    m4x4 VertexTransform = OrthographicProjection(0, (f32)WindowWidth, 0, (f32)WindowHeight);
    
    // TODO: These don't need to be set once a frame (only when window changes dimensionos)
    glUniformMatrix4fv(OpenGL->TransformUniformLocation, 1, GL_TRUE, &VertexTransform.E[0][0]);
    glViewport(0, 0, WindowWidth, WindowHeight);
    
    glScissor(0, 0, WindowWidth, WindowHeight);
    
    for (u8 *PushBufferAt = RenderCommands->PushBufferBase; 
         PushBufferAt < RenderCommands->PushBufferAt;)
    {
        render_entry_header *Header = (render_entry_header *)PushBufferAt;
        PushBufferAt += sizeof(render_entry_header);
        
        switch (Header->Type)
        {
            case RenderEntryType_Clear:
            {
                render_entry_clear *Entry = (render_entry_clear *)PushBufferAt;
                PushBufferAt += sizeof(render_entry_clear);
                
                v4 ClearColour = Entry->ClearColour;
                
                // We don't clear the depth buffer so we can use painters algorithm (order dependent) rendering
                glClearColor(ClearColour.r, ClearColour.g, ClearColour.b, ClearColour.a);
                glClear(GL_COLOR_BUFFER_BIT);
            } break;
            
            case RenderEntryType_Clip:
            {
                // Could instead have each type of entry specify an index into a clip rect array
                // and whenever the current set clip rect is different we call glScissor.
                // Maybe if you had lots of entries that reused the same clip rects this would be better.
                render_entry_clip *Entry = (render_entry_clip *)PushBufferAt;
                PushBufferAt += sizeof(render_entry_clip);
                
                glScissor(Entry->x, Entry->y, Entry->Width, Entry->Height);
            } break;
            case RenderEntryType_Line:
            {
#if 0
                render_entry_line *Entry = (render_entry_line *)PushBufferAt;
                PushBufferAt += sizeof(render_entry_line);
                
                u64 IndexOffset = Entry->IndexOffset * sizeof(u32); 
                GLsizei IndexCount = 2;
                
                // TODO: There is a range of supported line widths. Only width 1 is guaranteed to be supported; others depend on the implementation
                glLineWidth(Entry->LineWidth);
                glDrawElements(GL_LINES, IndexCount, GL_UNSIGNED_INT, (void *)IndexOffset);
#endif
            } break;
            
            case RenderEntryType_QuadOutline:
            {
#if 0
                /*
render_entry_quad_outline *Entry = (render_entry_quad_outline *)PushBufferAt;
PushBufferAt += sizeof(render_entry_quad_outline);
                u64 IndexOffset = Entry->IndexOffset * sizeof(u32); 
                glsizei IndexCount = 4 * 2;
                
                glLineWidth(Entry->LineWidth);
                glDrawElements(GL_LINES, IndexCount, GL_UNSIGNED_INT, IndexOffset);
                */
#endif
            } break;
            
            case RenderEntryType_TexturedQuads:
            {
                render_entry_textured_quads *Entry = (render_entry_textured_quads *)PushBufferAt;
                PushBufferAt += sizeof(render_entry_textured_quads);
                
                u64 IndexOffset = Entry->IndexOffset * sizeof(u32); 
                GLsizei IndexCount = Entry->QuadCount * 6;
                
                glBindTexture(GL_TEXTURE_2D, Entry->TextureHandle);
                // TODO:Use DrawElementsBaseVertex
                glDrawElements(GL_TRIANGLES, IndexCount, GL_UNSIGNED_INT, (void *)IndexOffset);
            } break;
        }
    }
}

void WriteImage(char *ImageFileName, u32 Width, u32 Height, u8 *Pixels, u16 BitsPerPixel, bool TopDown)
{
    u32 image_size = Width * Height * 4;
    
    s32 SignedHeight = Height;
    
    Bitmap_Header header = {};
    header.file_type = 0x4D42;
    header.file_size = sizeof(header) + image_size;
    header.file_offset = sizeof(header);
    header.header_size = sizeof(header) - 14; // just size of bitmapinfoheader not file header
    header.width = Width;
    header.height = Height;
    header.planes = 1;
    header.bits_per_pixel = BitsPerPixel;
    header.compression = 0;
    header.image_size = image_size;
    
    if (TopDown) 
    {
        header.height = -header.height;
    }
    
    FILE *file = fopen(ImageFileName, "wb");
    if (file) {
        fwrite(&header, sizeof(header), 1, file);
        fwrite(Pixels, image_size, 1, file);
        fclose(file);
    }
}

//u8 *OpenGLGetScreenPixels(u32 WindowWidth, u32 WindowHeight)
//{
//    // Read BGRA because this is what windows expects?
//    u8 *Pixels = (u8 *)calloc(4 * WindowWidth * WindowHeight, 1);
//    glReadPixels(0, 0,
//                 WindowWidth, WindowHeight,
//                 GL_BGRA,
//                 GL_UNSIGNED_BYTE,
//                 Pixels);
//    
//    return Pixels;
//}
