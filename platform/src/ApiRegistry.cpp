#include "pch.h"
#include "ApiRegistry.h"

HashTable<const char*, void*> gRegistries;
ILinearAllocator* gApplicationAllocator;

void ApiRegistrySet(const char *name, void *interf, u64 size)
{
    void* data = nullptr;
    bool contains = gRegistries.Get(name, &data);
    if (!contains)
    {
        data = gApplicationAllocator->Alloc(gApplicationAllocator->instance, size);
        memcpy(data, interf, size);
        gRegistries.Set(name, data);
    }
    else
    {
        memcpy(data, interf, size);
    }
}

void ApiRegistryRemove(const char *name)
{
    gRegistries.Remove(name);
}

void* ApiRegistryGet(const char *name)
{
    void* result = nullptr;
    gRegistries.Get(name, &result);
    return result;
}

APIRegistry CreateApiRegistry(AllocatorAPI* allocatorAPI, ILinearAllocator* applicationAllocator)
{
    APIRegistry registry = {};
    registry.Set = &ApiRegistrySet;
    registry.Get = &ApiRegistryGet;
    registry.Remove = &ApiRegistryRemove;

    gRegistries = CreateHashTable<const char*, void*>(allocatorAPI, 512);
    gApplicationAllocator = applicationAllocator;

    return registry;
}