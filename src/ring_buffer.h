#pragma once

#include "windows.h"

#include "base.h"

///////////////////////////////////////////////////////////////////////
// Ring Buffer
//

struct os_ring_buffer_memory 
{
    u8 *Address;
    u64 Size;
    void *Handle;
};

struct ring_buffer
{
    //   r   w
    // | 0123   |
    u8 *Memory;
    // These need to be u64 with the current implementation.
    // If we want to the to be u32 then the Capacity must be a power of two.
    u64 Read;   // Read % Capacity indexes the first byte availble for reading
    u64 Write;  // Write % Capacity indexes the first byte free for writing (one past the last byte for reading)
    u64 Capacity; 
    void *Handle;
};


u8 *Win32FindAddressForSize(HANDLE MapFileHandle, u64 Size)
{
    u8 *Address = (u8 *)MapViewOfFile(MapFileHandle, FILE_MAP_ALL_ACCESS, 0, 0, Size);
    if (Address) 
    {
        BOOL UnmapResult = UnmapViewOfFile(Address);
        Assert(UnmapResult);
    }
    
    return Address;
}

bool OS_AllocateRingBufferMemory(os_ring_buffer_memory *RingBufferMemory, u64 Size)
{
    // MapViewOfFileEx takes a base address which must be on an memory allocation granularity boundary (64 KB).
    // So the ring size must be a multiple of 64 KB (as the adjacent mapped memory is contiguous).
    Assert((Size & 0xFFFF) == 0);
    Assert(Size > 0);
    
    bool Success = false;
    
    u64 MirroredSize = Size * 2;
    HANDLE MapFileHandle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 
                                              (DWORD)(MirroredSize >> 32), (DWORD)(MirroredSize & 0xFFFFFFFFu), 
                                              NULL);
    if (MapFileHandle)
    {
        // On windows we use MapViewOfFile to find a region that can fit our two adjacent memory regions.
        // Then we map these two separately. This introduces a race condition where another thread may allocate
        // the memory we plan to map inbetween our MapViewOfFileEx calls.
        // Windows 10 has VirtualAlloc2 which is allows to reserving both regions designed for explicitly creating // a mirrored ring buffer.
        for (u32 Tries = 0; Tries < 10; ++Tries)
        {
            u8 *Address = Win32FindAddressForSize(MapFileHandle, MirroredSize);
            if (Address) 
            {
                u8 *Address1 = (u8 *)MapViewOfFileEx(MapFileHandle, FILE_MAP_ALL_ACCESS, 0, 0, 
                                                     Size, Address);
                if (Address1 == Address) 
                {
                    u8 *Address2 = (u8 *)MapViewOfFileEx(MapFileHandle, FILE_MAP_ALL_ACCESS, 0, 0,
                                                         Size, Address + Size);
                    if (Address2 == Address + Size) 
                    {
                        // Windows zeros the memory
                        RingBufferMemory->Address = Address;
                        RingBufferMemory->Size = Size;
                        RingBufferMemory->Handle = MapFileHandle;
                        
                        Success = true;
                        break;
                    }
                    else
                    {
                        BOOL UnmapResult = UnmapViewOfFile(Address1);
                        Assert(UnmapResult);
                    }
                }
                
                // Some other thread took the memory we wanted so try again
                if (GetLastError() == ERROR_INVALID_ADDRESS) 
                {
                    continue;
                } 
            }
            
            break;
        }
        
        if (!Success)
        {
            BOOL CloseResult = CloseHandle(MapFileHandle);
            Assert(CloseResult);
        }
    }
    
    return Success;
}

void OS_FreeRingBufferMemory(u8 *Address, u64 Size, void *Handle)
{
    Assert(Address);
    Assert(Size);
    Assert(Handle);
    
    BOOL UnmapResult = UnmapViewOfFile(Address);
    Assert(UnmapResult);
    UnmapResult = UnmapViewOfFile(Address + Size);
    Assert(UnmapResult);
    BOOL CloseResult = CloseHandle((HANDLE)Handle);
    Assert(CloseResult);
}



// The buffer size needs to be a power of two to ensure that the Read and Write indexes have the correct
// behaviour when they overflow.
// For example. 
// Using 4 bit indexes (wrap after 15, for simplicity) a buffer size of 7.
// When the Read index is at 15 it will map to the slot 15 % 7 = 1.
// After being incremented it will be 15 + 1 = 0. Which has moved backwards from where it should be.

// Using a power of two buffer size solves this.
// When the Read index is at 15 it will map to the slot 15 % 8 = 7.
// After being incremented it will be 15 + 1 = 0. Which has correctly wrapped around the buffer.

// Using power of two buffer sizes also allows us to use an AND rather than modulo to compute buffer slots.

// An alternative is to use u64 for the indexes (which will take 1000 years to overflow while processing 1 GB per second).
bool InitRingBuffer(ring_buffer *RingBuffer, u64 Size)
{
    Assert(IsPowerOfTwo(Size));
    
    bool Success = false;
    
    os_ring_buffer_memory BufferMemory = {};
    if (OS_AllocateRingBufferMemory(&BufferMemory, Size))
    {
        RingBuffer->Memory = BufferMemory.Address;
        RingBuffer->Capacity = BufferMemory.Size;
        RingBuffer->Handle = BufferMemory.Handle;
        RingBuffer->Read = 0;
        RingBuffer->Write = 0;
        
        Success = true;
    }
    
    return Success;
}


void FreeRingBuffer(ring_buffer *RingBuffer)
{
    Assert(RingBuffer->Memory);
    Assert(RingBuffer->Capacity); 
    Assert(RingBuffer->Handle);
    OS_FreeRingBufferMemory(RingBuffer->Memory, RingBuffer->Capacity, RingBuffer->Handle);
}

u64 RingBufferFilledSize(ring_buffer *RingBuffer)
{
    Assert(RingBuffer->Read <= RingBuffer->Write);
    Assert(RingBuffer->Write <= RingBuffer->Read + RingBuffer->Capacity);
    
    return RingBuffer->Write - RingBuffer->Read;
}

u64 RingBufferFreeSize(ring_buffer *RingBuffer)
{
    Assert(RingBuffer->Read <= RingBuffer->Write);
    Assert(RingBuffer->Write <= RingBuffer->Read + RingBuffer->Capacity);
    
    return RingBuffer->Capacity - RingBufferFilledSize(RingBuffer);
}

u8 *RingBufferReadPtr(ring_buffer *RingBuffer)
{
    return RingBuffer->Memory + (RingBuffer->Read % RingBuffer->Capacity);
}

void RingBufferAdvanceReadPtr(ring_buffer *RingBuffer, u64 Size)
{
    RingBuffer->Read += Size;
    
    Assert(RingBuffer->Read <= RingBuffer->Write);
    Assert(RingBuffer->Write <= RingBuffer->Read + RingBuffer->Capacity);
}

u8 *RingBufferWritePtr(ring_buffer *RingBuffer)
{
    return RingBuffer->Memory + (RingBuffer->Write % RingBuffer->Capacity);
}

void RingBufferWriteData(ring_buffer *RingBuffer, u64 Size, u8 *Data)
{
    Assert(Size <= RingBufferFreeSize(RingBuffer));
    Assert(Data);
    
    u8 *WritePtr = RingBufferWritePtr(RingBuffer);
    MemoryCopy(Size, Data, WritePtr);
    RingBuffer->Write += Size;
    
    Assert(RingBuffer->Read <= RingBuffer->Write);
    Assert(RingBuffer->Write <= RingBuffer->Read + RingBuffer->Capacity);
}
