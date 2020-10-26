#include "pch.h"
#include "Profiler.h"
#include "Platform.h"
#include "Allocator.h"
#include "ApiRegistry.h"

static APIRegistry* gAPIRegistry = nullptr;
static TimeAPI* gTimeAPI = nullptr;

struct ProfilerState
{
    ILinearAllocator* textAllocator;
    DynamicArray<ProfileData> scopes;
};

static ProfilerState* gState = nullptr;

void ProfilerInit(AllocatorAPI* allocatorAPI, ILinearAllocator* applicationAllocator)
{
    gState = (ProfilerState*) applicationAllocator->Alloc(applicationAllocator->instance, sizeof(ProfilerState));
    // Update state pointer in api
    ProfilerAPI* api = (ProfilerAPI*) gAPIRegistry->Get(PROFILER_API_NAME);
    api->state = (void*) gState;

    gState->textAllocator = allocatorAPI->CreateLinearAllocator(Megabyte(512), Megabyte(1));
    gState->scopes = CreateDynamicArray<ProfileData>(allocatorAPI);
}

u64 ProfilerBegin()
{
    ASSERT(gState, "Profiler API has not been initialized!");
    return gTimeAPI->GetPerformanceCounterTimeNanoseconds();
}

void ProfilerEnd(const char* name, u64 startNanoSeconds)
{
    ASSERT(gState, "Profiler API has not been initialized!");
    u64 len = strlen(name);
    char* text = (char*) gState->textAllocator->Alloc(gState->textAllocator->instance, len + 1);
    memcpy(text, name, len);

    u64 endNanoSeconds = gTimeAPI->GetPerformanceCounterTimeNanoseconds();
    u64 durationNanoSeconds = endNanoSeconds - startNanoSeconds;

    ProfileData profileData = { text, startNanoSeconds, durationNanoSeconds };
    gState->scopes.Append(profileData);
}

u32 ProfilerGetProfileDatas(const ProfileData** outBaseAddress)
{
    ASSERT(gState, "Profiler API has not been initialized!");
    *outBaseAddress = gState->scopes.data;
    return gState->scopes.length;
}

void ProfilerClearData()
{
    ASSERT(gState, "Profiler API has not been initialized!");
    gState->textAllocator->ClearMemory(gState->textAllocator->instance);
    gState->scopes.Clear();
}

void ProfilerShutdown(AllocatorAPI* allocatorAPI)
{
    ASSERT(gState, "Profiler API has not been initialized!");
    allocatorAPI->DestroyLinearAllocator(gState->textAllocator);
    DestroyDynamicArray<ProfileData>(&gState->scopes, allocatorAPI);
}

void RegisterProfilerAPI(APIRegistry* registry, bool reload)
{
    gAPIRegistry = registry;
    gTimeAPI = ((PlatformAPI*) registry->Get(PLATFORM_API_NAME))->timeAPI;

    ProfilerAPI profilerAPI = {};
    if (reload)
    {
        ProfilerAPI* api = (ProfilerAPI*) registry->Get(PROFILER_API_NAME);
        ASSERT(api, "Can't find API on reload");
        gState = (ProfilerState*) api->state;
    }

    profilerAPI.state = (void*) gState;
    profilerAPI.Init = ProfilerInit;
    profilerAPI.Begin = ProfilerBegin;
    profilerAPI.End = ProfilerEnd;
    profilerAPI.GetProfileDatas = ProfilerGetProfileDatas;
    profilerAPI.Shutdown = ProfilerShutdown;
    profilerAPI.ClearData = ProfilerClearData;

    registry->Set(PROFILER_API_NAME, &profilerAPI, sizeof(ProfilerAPI));
}