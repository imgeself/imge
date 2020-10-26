#include "pch.h"
#include "Platform.h"
#include "Allocator.h"
#include <stdlib.h>

VirtualMemoryAPI* gVirtualMemoryApi = nullptr;

void* LinearAlloc(LinearAllocator* inst, u64 size)
{
    ASSERT((inst->startOffset + size) <= inst->reservedSize, "Requested size is bigger than reserved memory");

    if ((inst->startOffset + size) > inst->commitedSize)
    {
        u64 commitSize = inst->commitedSize;
        if ((commitSize + inst->commitedSize) > inst->reservedSize)
        {
            commitSize = inst->reservedSize - inst->commitedSize;
        }
        gVirtualMemoryApi->Alloc((void*) (inst->pointer + inst->commitedSize), commitSize, VA_COMMIT);
        inst->commitedSize += commitSize;
    }

    u8* result = inst->pointer + inst->startOffset;
    inst->startOffset += size;
    return result;
}

void LinearFree(LinearAllocator* inst, u64 size)
{
    ASSERT(inst->startOffset >= 0, "Start offset cannot be negative, you probably freed more than you allocated");
    inst->startOffset -= size;

    if (inst->commitedSize > VM_PAGE_SIZE && inst->startOffset <= inst->commitedSize / 4)
    {
        gVirtualMemoryApi->Free((void*) (inst->pointer + (inst->commitedSize / 2)), inst->commitedSize / 2, VF_DECOMMIT);
        inst->commitedSize /= 2;
    }
}

void LinearClearMemory(LinearAllocator* inst)
{
    memset(inst->pointer, 0, inst->commitedSize);
    inst->startOffset = 0;
}

ILinearAllocator* CreateLinearAllocator(u64 reserveSize, u64 initialCommitSize)
{
    ASSERT(reserveSize >= initialCommitSize, "Reserve size must be greater than or equal to commit size");

    LinearAllocator* linearAllocatorInstance = (LinearAllocator*) malloc(sizeof(LinearAllocator));
    linearAllocatorInstance->commitedSize = initialCommitSize;
    linearAllocatorInstance->startOffset = 0;
    linearAllocatorInstance->reservedSize = reserveSize;

    u8* data = nullptr;
    if (reserveSize == initialCommitSize)
    {
        data = (u8*) gVirtualMemoryApi->Alloc(0, reserveSize, VA_RESERVE | VA_COMMIT);
    }
    else
    {
        data = (u8*) gVirtualMemoryApi->Alloc(0, reserveSize, VA_RESERVE);
        gVirtualMemoryApi->Alloc((void*) data, initialCommitSize, VA_COMMIT);
    }

    if (data == nullptr)
    {
        return nullptr;
    }
    linearAllocatorInstance->pointer = data;

    ILinearAllocator* linearAllocator = (ILinearAllocator*) malloc(sizeof(ILinearAllocator));
    linearAllocator->instance = linearAllocatorInstance;
    linearAllocator->Alloc = &LinearAlloc;
    linearAllocator->Free = &LinearFree;
    linearAllocator->ClearMemory = &LinearClearMemory;

    return linearAllocator;
}

void DestroyLinearAllocator(ILinearAllocator* allocator)
{
    gVirtualMemoryApi->Free(allocator->instance->pointer, 0, VF_RELEASE);
    free(allocator->instance);
    free(allocator);
    allocator = nullptr;
}

AllocatorAPI CreateAllocatorAPI(VirtualMemoryAPI* virtualMemoryAPI)
{
    gVirtualMemoryApi = virtualMemoryAPI;

    AllocatorAPI api = {};
    api.CreateLinearAllocator = &CreateLinearAllocator;
    api.DestroyLinearAllocator = &DestroyLinearAllocator;

    return api;
}