#pragma once

struct ILinearAllocator;

struct Entity
{
    Handle handle;
};

struct Component
{
    u32 componentIndex;
};

struct EntityContext;

#define MAX_COMPONENT_TYPE_COUNT 64
#define MAX_SYSTEM_COMPONENT_TYPE_COUNT 8

struct EntitySignature
{
    u64 componentMaskBits[MAX_COMPONENT_TYPE_COUNT / 64];
};

struct EntitySystemUpdateArray
{
    void* componentData[MAX_SYSTEM_COMPONENT_TYPE_COUNT];
    u32 length;
};

struct EntitySystemUpdateSet
{
    EntitySystemUpdateArray* arrays;
    u32 numArrays;
};

struct IEntitySystem
{
    u32 numComponent;
    Component components[MAX_SYSTEM_COMPONENT_TYPE_COUNT];
    void* userData;

    void (*Update)(EntityContext* context, EntitySystemUpdateSet* updateData, void* userData);
    bool (*Filter)(EntityContext* context, Component* components, u32 numComponents, EntitySignature signature);
};

#define ENTITY_API_NAME "EntityAPI"

struct EntityAPI
{
    EntityContext* (*CreateContext)(ILinearAllocator* allocator);

    Entity (*CreateEntity)(EntityContext* context);
    Entity (*CreateEntityWithComponents)(EntityContext* context, Component* components, void** componentDatas, u32 numComponents);

    // TODO: Implement DestroyEntity

    Component (*RegisterComponent)(EntityContext* context, const char* componentName, u32 componentSize);
    Component (*GetComponentFromName)(EntityContext* context, const char* componentName);

    // TODO: Implement AddComponent/RemoveComponent

    void (*PushSystem)(EntityContext* context, IEntitySystem* entitySystem);
    void (*RunSystems)(EntityContext* context, ILinearAllocator* frameAllocator);
};

static inline bool HasEntitySignatureComponent(const EntitySignature* signature, Component component)
{
    return signature->componentMaskBits[component.componentIndex / 64] & (1ULL << (component.componentIndex % 64));
}