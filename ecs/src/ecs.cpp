#include "pch.h"
#include "ecs.h"
#include "Allocator.h"
#include "ApiRegistry.h"

static APIRegistry* gAPIRegistry = nullptr;

struct ArchetypeComponent
{
    u32 componentIndex;
    u32 componentSize;
};

struct Archetype
{
    // Get component types from signature
    ILinearAllocator* allocator;
    u32 entityCount;

    // TODO: Do we need to store these? Can't we just extract these information from component mask?
    ArchetypeComponent components[MAX_COMPONENT_TYPE_COUNT];
    u32 componentCount;
};

struct EntityData
{
    u32 archetypeIndex;
};

struct EntityContext
{
    HandlePool entityPool;

    EntitySignature archetypeSignatures[MAX_COMPONENT_TYPE_COUNT];
    Archetype archetypes[MAX_COMPONENT_TYPE_COUNT];
    u32 createdArchetypeCount;

    u32 componentSizes[MAX_COMPONENT_TYPE_COUNT];
    u32 registeredComponentCount;
    HashTable<const char*, u32> componentTable;
    DynamicArray<IEntitySystem> systems;
};



EntityContext* CreateEntityContext(ILinearAllocator* allocator)
{
    EntityContext* context = (EntityContext*) allocator->Alloc(allocator->instance, sizeof(EntityContext));
    AllocatorAPI* allocatorAPI = (AllocatorAPI*) gAPIRegistry->Get(ALLOCATOR_API_NAME);

    context->entityPool = InitHandlePool(allocator, 4096, sizeof(u32));
    context->componentTable = CreateHashTable<const char*, u32>(allocatorAPI, 512);
    context->systems = CreateDynamicArray<IEntitySystem>(allocatorAPI);

    // Index 0 is the default archetype which is a entity has no components
    context->archetypeSignatures[0] = {0};
    // We don't need a allocator because default archetype has no components
    Archetype archetype = {};
    archetype.allocator = nullptr;
    archetype.entityCount = 0;
    archetype.componentCount = 0;
    context->archetypes[0] = archetype;

    return context;
}

Entity CreateEntity(EntityContext* context)
{
    Handle handle = ObtainNewHandleFromPool(&context->entityPool);
    EntityData* entityData = (EntityData*) AccessDataFromHandlePool(&context->entityPool, handle);
    entityData->archetypeIndex = 0;

    return Entity { handle };
}


int CompareComponents(const void * a, const void * b)
{
  return ( ((Component*)a)->componentIndex - ((Component*)b)->componentIndex );
}

Entity CreateEntityWithComponents(EntityContext* context, Component* components, void** componentDatas, u32 numComponents)
{
    Handle handle = ObtainNewHandleFromPool(&context->entityPool);
    

    EntitySignature signature = {};
    for (u32 i = 0; i < numComponents; ++i)
    {
        u32 componentIndex = components[i].componentIndex;
        signature.componentMaskBits[componentIndex / 64] = signature.componentMaskBits[componentIndex / 64] | (1ULL << (componentIndex % 64));
    }

    // Search an archetype that has matching signature
    u32 archetypeIndex = NULL_INDEX;
    for (u32 i = 0; i < context->createdArchetypeCount; ++i)
    {
        EntitySignature* sig = context->archetypeSignatures + i;
        if (memcmp(&signature, sig, sizeof(EntitySignature)) == 0)
        {
            archetypeIndex = i;
            break;
        }
    }

    // If not, create archetype
    if (archetypeIndex == NULL_INDEX)
    {
        AllocatorAPI* allocatorAPI = (AllocatorAPI*) gAPIRegistry->Get(ALLOCATOR_API_NAME);
        u32 index = context->createdArchetypeCount++;
        context->archetypeSignatures[index] = signature;
        ILinearAllocator* allocator = allocatorAPI->CreateLinearAllocator(Megabyte(512), Megabyte(1));
        Archetype archetype = {};
        archetype.allocator = allocator;
        archetype.entityCount = 0;

        Component* sortedComponents = (Component*) alloca(sizeof(Component) * numComponents);
        memcpy(sortedComponents, components, sizeof(Component) * numComponents);
        qsort(sortedComponents, numComponents, sizeof(Component), CompareComponents);
        for (u32 i = 0; i < numComponents; ++i)
        {
            Component* comp = components + i;
            u32 componentSize = context->componentSizes[comp->componentIndex];
            archetype.components[i] = ArchetypeComponent{ comp->componentIndex, componentSize };
        }
        archetype.componentCount = numComponents;

        context->archetypes[index] = archetype;
        archetypeIndex = index;
    }

    ASSERT(archetypeIndex < context->createdArchetypeCount, "Invalid archetype index");
    Archetype* archetype = context->archetypes + archetypeIndex;
    ASSERT(archetype->componentCount == numComponents, "Component counts don't match");
    u64 allocationSize = 0;
    for (u32 i = 0; i < archetype->componentCount; ++i)
    {
        ArchetypeComponent* archComp = archetype->components + i;
        u32 componentSize = context->componentSizes[archComp->componentIndex];
        allocationSize += componentSize;
    }

    // Allocate memory for all components
    archetype->allocator->Alloc(archetype->allocator->instance, allocationSize);

    // Insert component data
    u64 leftComponentsSize = 0;
    u64 rightComponentsSize = allocationSize;
    for (u32 i = 0; i < archetype->componentCount; ++i)
    {
        ArchetypeComponent* archComp = archetype->components + i;
        u32 componentSize = context->componentSizes[archComp->componentIndex];
        u32 entityCount = archetype->entityCount;

        leftComponentsSize += componentSize;
        rightComponentsSize -= componentSize;
        u8* data = archetype->allocator->instance->pointer;
        u8* start = data + (leftComponentsSize * entityCount) + (leftComponentsSize - componentSize);
        memmove(start + componentSize, start, rightComponentsSize * entityCount);

        // Find component data
        u32 dataIndex = NULL_INDEX;
        for (u32 componentDataIndex = 0; componentDataIndex < numComponents; ++componentDataIndex)
        {
            Component* com = components + componentDataIndex;
            if (archComp->componentIndex == com->componentIndex)
            {
                dataIndex = componentDataIndex;
            }
        }
        ASSERT(dataIndex != NULL_INDEX && dataIndex < numComponents, "Invalid component data index");

        void* componentData = componentDatas[dataIndex];
        memcpy((void*) start, componentData, componentSize);
    }
    archetype->entityCount++;

    EntityData* entityData = (EntityData*) AccessDataFromHandlePool(&context->entityPool, handle);
    entityData->archetypeIndex = archetypeIndex;

    return Entity { handle };

}


Component RegisterComponent(EntityContext* context, const char* componentName, u32 componentSize)
{
    u32 existingComponentIndex;
    bool isRegistered = context->componentTable.Get(componentName, &existingComponentIndex);
    if (isRegistered)
    {
        return Component{ existingComponentIndex };
    }
    else
    {
        u32 componentIndex = context->registeredComponentCount++;

        context->componentSizes[componentIndex] = componentSize;
        context->componentTable.Set(componentName, componentIndex);

        return Component{ componentIndex };
    }
}

void PushSystem(EntityContext* context, IEntitySystem* entitySystem)
{
    context->systems.Append(*entitySystem);
}

void RunSystems(EntityContext* context, ILinearAllocator* frameAllocator)
{

    u32 numSystems = context->systems.length;
    for (u32 systemIndex = 0; systemIndex < numSystems; ++systemIndex)
    {
        IEntitySystem system = context->systems[systemIndex];

        u32 matchedArchetypeCount = 0;
        u32 numArchetypes = context->createdArchetypeCount;
        for (u32 archIndex = 0; archIndex < numArchetypes; ++archIndex)
        {
            EntitySignature signature = context->archetypeSignatures[archIndex];
            bool signatureOkay = system.Filter(context, system.components, system.numComponent, signature);
            if (signatureOkay) {
                matchedArchetypeCount++;
            }
        }

        if (matchedArchetypeCount > 0)
        {
            EntitySystemUpdateSet updateSet = {};
            updateSet.numArrays = matchedArchetypeCount;
            EntitySystemUpdateArray* array = (EntitySystemUpdateArray*) frameAllocator->Alloc(frameAllocator->instance, matchedArchetypeCount * sizeof(EntitySystemUpdateArray));
            updateSet.arrays = array;

            for (u32 archIndex = 0; archIndex < numArchetypes; ++archIndex)
            {
                EntitySignature signature = context->archetypeSignatures[archIndex];
                bool signatureOkay = system.Filter(context, system.components, system.numComponent, signature);
                if (signatureOkay) {
                    Archetype* archetype = context->archetypes + archIndex;

                    EntitySystemUpdateArray* updateArray = array++;
                    updateArray->length = archetype->entityCount;

                    for (u32 systemCompIndex = 0; systemCompIndex < system.numComponent; ++systemCompIndex)
                    {
                        Component systemComponent = system.components[systemCompIndex];

                        // Find system component in archetype components
                        // TODO: Archetype components is sorted. Maybe implement binary search here.
                        u32 leftComponentSize = 0;
                        for (u32 archCompIndex = 0; archCompIndex < archetype->componentCount; ++archCompIndex)
                        {
                            ArchetypeComponent archetypeComponent = archetype->components[archCompIndex];
                            if (archetypeComponent.componentIndex == systemComponent.componentIndex)
                            {
                                u8* data = archetype->allocator->instance->pointer + (archetype->entityCount * leftComponentSize);
                                updateArray->componentData[systemCompIndex] = data;
                                break;
                            }
                            leftComponentSize += archetypeComponent.componentSize;
                        }
                    }
                }
            }

            system.Update(context, &updateSet, system.userData);
        }
    }

}

extern "C"
{
    MODULE_EXPORT void LoadPlugin(APIRegistry* registry, bool reload)
    {
        gAPIRegistry = registry;
        EntityAPI entityAPI = {};
        entityAPI.CreateContext = CreateEntityContext;
        entityAPI.CreateEntity = CreateEntity;
        entityAPI.CreateEntityWithComponents = CreateEntityWithComponents;
        entityAPI.RegisterComponent = RegisterComponent;
        entityAPI.PushSystem = PushSystem;
        entityAPI.RunSystems = RunSystems;
        

        registry->Set(ENTITY_API_NAME, &entityAPI, sizeof(EntityAPI));
    }

    MODULE_EXPORT void UnloadPlugin(APIRegistry* registry, bool reload)
    {
        registry->Remove(ENTITY_API_NAME);
    }
}