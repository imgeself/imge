#pragma once

struct ILinearAllocator;

#define SYSTEM_API_NAME "SystemAPI"

struct SystemAPI
{
    void* state;

    void (*Init)(ILinearAllocator* applicationAllocator);
    bool (*Update)();
    void (*Shutdown)();
};
