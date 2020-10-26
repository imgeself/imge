#pragma once

#include "Core.h"
#include "Allocator.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <math.h>

#define ASSERT(x, string) { if(!(x)) { printf("Assertion Failed: %s\n", string); __debugbreak(); } }
#define BIT(x) (1u << (x))

#define Kilobyte(x) (x) * 1024
#define Megabyte(x) (x) * 1024 * 1024
#define Gigabyte(x) (x) * 1024 * 1024 * 1024

#define VM_PAGE_SIZE 4 * 1024

#include "DynamicArray.h"
#define XXH_INLINE_ALL
#include "xxhash.h"
typedef XXH64_hash_t Hash64;
#include "HashTable.h"
#include "Keycode.h"


// Memory
inline void SwapMemory(void* ptr1, void* ptr2, u64 size)
{
    void* tempMemory = alloca(size);
    memcpy(tempMemory, ptr1, size);
    memcpy(ptr1, ptr2, size);
    memcpy(ptr2, tempMemory, size);
}

// Temporary allocator
struct TempAllocator
{
    char buffer[1024] = {0};
    u32 start = 0;

    inline char* Printf(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        char* result = buffer + start;
        vsprintf(result, format, args);
        va_end(args);
        start += strlen(result) + 1;

        return result;
    }

    inline char* Alloc(u32 size)
    {
        char* result = buffer + start;
        start += size;
        return result;
    }

    void Clear()
    {
        memset(buffer, 0, 1024);
        start = 0;
    }
};

// String operations
inline const char* AllocateString(ILinearAllocator* allocator, const char* string)
{
    u64 size = strlen(string) + 1;
    char* pointer = (char*) allocator->Alloc(allocator->instance, size);
    memcpy(pointer, string, size);
    return pointer;
}

// Return directory string without filename and last backslash
inline void PathRemoveFilename(char* path, bool windowsPath = false)
{
    // Search extension dot. If there is we can remove filename section from the path
    // If not, we assume path is a directory and we do nothing
    if (strrchr(path, '.'))
    {
        if (windowsPath)
        {
            *strrchr(path, '\\') = '\0';
        }
        else
        {
            *strrchr(path, '/') = '\0';
        }
    }
    
}

#define NULL_INDEX 0xFFFFFFFF
#define INVALID_HANDLE_INDEX 0xFFFF

struct Handle
{
    u32 index : 16;
    u32 generation : 16;
};

inline bool operator==(Handle h1, Handle h2)
{
    return h1.index == h2.index && h1.generation == h2.generation;
}

inline bool operator!=(Handle h1, Handle h2)
{
    return h1.index != h2.index || h1.generation != h2.generation;
}

const Handle INVALID_HANDLE = { .index = INVALID_HANDLE_INDEX, .generation = 0xFFFF };

struct HandlePool 
{
    u8* data;
    u8* generations;
    u32 freeIndex;
    u32 elementCount;
    u32 resourceSize;
};

struct EmptyPoolData
{
    u32 nextFreeIndex;
};

inline HandlePool InitHandlePool(ILinearAllocator* allocator, u32 elementCount, u32 resourceSize)
{
    ASSERT(elementCount < 0xFFFF, "The element count cannot be more than max 24 bit value");
    ASSERT(resourceSize >= 4, "Element size cannot be less than 4 bytes");

    HandlePool handlePool = {};
    handlePool.elementCount = elementCount;
    handlePool.data = (u8*) allocator->Alloc(allocator->instance, elementCount * resourceSize);
    handlePool.generations = (u8*) allocator->Alloc(allocator->instance, elementCount);
    handlePool.resourceSize = resourceSize;
    handlePool.freeIndex = 0;

    for (u32 i = 0; i < (elementCount - 1); ++i)
    {
        EmptyPoolData* emptyData = (EmptyPoolData*) (handlePool.data + (i * resourceSize));
        emptyData->nextFreeIndex = i + 1;
    }

    // Last element
    EmptyPoolData* emptyData = (EmptyPoolData*) (handlePool.data + ((elementCount - 1) * resourceSize));
    emptyData->nextFreeIndex = NULL_INDEX;

    return handlePool;
}

inline Handle ObtainNewHandleFromPool(HandlePool* pool)
{
    ASSERT(pool->freeIndex < pool->elementCount && pool->freeIndex != NULL_INDEX, "Not enough space in handle pool");

    u32 freeIndex = pool->freeIndex;
    Handle result;
    result.index = freeIndex;
    result.generation = pool->generations[freeIndex];

    EmptyPoolData* emptyData = (EmptyPoolData*) (pool->data + (freeIndex * pool->resourceSize));
    u32 newFreeIndex = emptyData->nextFreeIndex;
    pool->freeIndex = newFreeIndex;

    return result;
}

inline void* AccessDataFromHandlePool(HandlePool* pool, Handle handle)
{
    if (handle.index != INVALID_HANDLE_INDEX)
    {
        u8 generationCount = pool->generations[handle.index];
        ASSERT(generationCount == handle.generation, "Accessing deleted data in HandePool");
        return (pool->data + (handle.index * pool->resourceSize));
    }

    return nullptr;
}

inline void ReleaseHandle(HandlePool* pool, Handle handle)
{
    pool->generations[handle.index]++;
    EmptyPoolData* emptyData = (EmptyPoolData*) (pool->data + (handle.index * pool->resourceSize));
    emptyData->nextFreeIndex = pool->freeIndex;
    pool->freeIndex = handle.index;
}

