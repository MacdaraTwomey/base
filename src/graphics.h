#pragma once

#include "base.h"

enum render_entry_type
{
    RenderEntryType_Clear,
    RenderEntryType_Clip,
    RenderEntryType_Line,
    RenderEntryType_QuadOutline,
    RenderEntryType_TexturedQuads,
};

struct render_entry_header
{
    render_entry_type Type;
};

struct render_entry_clear
{
    v4 ClearColour;
};

struct render_entry_clip
{
    s32 x;
    s32 y;
    s32 Width;
    s32 Height;
};

struct render_entry_line
{
    u32 IndexOffset;
    f32 LineWidth;
};

//
//struct render_entry_quad_outline
//{
//u32 IndexOffset;
//f32 LineWidth;
//};
//

// A set of quads using one texture
struct render_entry_textured_quads
{
    u32 IndexOffset;
    u32 QuadCount;
    u32 TextureHandle;
};

struct vertex
{
    v2 P;
    v2 UV;
    u32 Colour;
};

// TODO: Render Entry coalescing
struct render_commands
{
    vertex *VertexArray;
    u32 VertexCount;
    u32 MaxVertexCount;
    
    u32 *IndexArray;
    u32 IndexCount;
    u32 MaxIndexCount;
    
    u8 *PushBufferBase;
    u8 *PushBufferAt;
    u32 PushBufferSize;
    
    u32 WhiteTextureHandle;
    
    // Combine textured quad entries that use the same texture (i.e. almost all entries).
    // This will only be non-null if the last entry in the push buffer was a render_entry_textured_quads,
    // so as to not break the order dependence of render commands.
    // Modified when PushQuad or PushEntry is called.
    render_entry_textured_quads *LastTexturedQuadEntry;
};

struct opengl
{
    u32 Program;
    u32 VAO;
    u32 VBO;
    u32 EBO;
    s32 TransformUniformLocation;
    u32 WhiteTexture;
    
    render_commands RenderCommands;
    
    // TODO: See how much is realistically needed
    // This could be untyped to separate opengl and renderer
    vertex VertexArray[KB(32)];
    //u32 IndexArray[ArrayCount(VertexArray) * 6 / 4];
    u32 IndexArray[KB(32)];
    
    u8 PushBufferMemory[KB(128)];
};

#pragma pack(push, 1)
struct Bitmap_Header
{
    // Bitmap file header
    u16 file_type;
    u32 file_size;
    u16 reserved1;
    u16 reserved2;
    u32 file_offset;
    
    // bitmap info header
    u32 header_size;
    s32 width;
    s32 height;
    u16 planes;
    u16 bits_per_pixel;
    u32 compression;
    u32 image_size;
    s32 x_pixels_per_meter;
    s32 y_pixels_per_meter;
    u32 colours_used;
    u32 colours_important;
};
#pragma pack(pop)