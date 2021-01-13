#pragma once

struct TimeAPI;
struct AllocatorAPI;
struct APIRegistry;

#define PROFILER_API_NAME "ProfilerAPI"

struct ProfileData
{
    const char* name;
    u64 startNanoseconds;
    u64 durationNanoseconds;
};

struct ProfilerAPI
{
    void* state;

    void (*Init)(AllocatorAPI* allocatorAPI, ILinearAllocator* applicationAllocator);
    u64 (*Begin)();
    void (*End)(const char* name, u64 startNanoSeconds);
    u32 (*GetProfileDatas)(const ProfileData** outBaseAddress);
    void (*ClearData)();
    void (*Shutdown)(AllocatorAPI* allocatorAPI);
};

struct ProfileScope
{
    const char* name;
    ProfilerAPI* api;
    u64 startNanoSeconds;

    inline ProfileScope(ProfilerAPI* profilerAPI, const char* scopeName)
    {
        name = scopeName;
        api = profilerAPI;
        startNanoSeconds = api->Begin();
    }

    inline ~ProfileScope()
    {
        api->End(name, startNanoSeconds);
    }

};

#define PROFILE(profilerAPI, name, line) ProfileScope scope##line(profilerAPI, name);
#define PROFILE_SCOPE(profilerAPI, name) PROFILE(profilerAPI, name, __LINE__)
#define PROFILE_FUNCTION(profilerAPI) PROFILE_SCOPE(profilerAPI, __FUNCTION__)