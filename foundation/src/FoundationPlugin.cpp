#include "pch.h"
#include "Log.h"
#include "ApiRegistry.h"
#include "Profiler.h"

extern void RegisterProfilerAPI(APIRegistry* registry, bool reload);
extern void RegisterLogAPI(APIRegistry* registry, bool reload);

extern "C"
{
    MODULE_EXPORT void LoadPlugin(APIRegistry* registry, bool reload)
    {
        RegisterLogAPI(registry, reload);
        RegisterProfilerAPI(registry, reload);
    }

    MODULE_EXPORT void UnloadPlugin(APIRegistry* registry, bool reload)
    {
        // NOTE: Do we need to unload foundation plugin? I don't think so.
    }
}