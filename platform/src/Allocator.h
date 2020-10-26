#pragma once

#define ALLOCATOR_API_NAME "AllocatorAPI"

struct VirtualMemoryAPI;

struct LinearAllocator
{
    u8* pointer;
    u64 startOffset;
    u64 reservedSize;
    u64 commitedSize;
};

struct ILinearAllocator
{
    LinearAllocator* instance;

    void* (*Alloc)(LinearAllocator* inst, u64 size);
    void (*Free)(LinearAllocator* inst, u64 size);
    void (*ClearMemory)(LinearAllocator* inst);
};

struct AllocatorAPI
{
    ILinearAllocator* (*CreateLinearAllocator)(u64 reserveSize, u64 initialCommitSize);
    void (*DestroyLinearAllocator)(ILinearAllocator* object);
};

AllocatorAPI CreateAllocatorAPI(VirtualMemoryAPI* virtualMemoryAPI);