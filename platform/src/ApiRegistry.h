#pragma once

struct ILinearAllocator;
struct AllocatorAPI;

struct APIRegistry
{
    void (*Set)(const char *name, void *interf, u64 size);
    void (*Remove)(const char *name);
    void* (*Get)(const char *name);
};

APIRegistry CreateApiRegistry(AllocatorAPI* allocatorAPI, ILinearAllocator* applicationAllocator);