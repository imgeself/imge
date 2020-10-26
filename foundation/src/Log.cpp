#include "pch.h"
#include "Log.h"
#include "Allocator.h"
#include "ApiRegistry.h"
#include "Platform.h"

static APIRegistry* gAPIRegistry = nullptr;
static PlatformAPI* gPlatformAPI = nullptr;

struct LogAPIState
{
    ILinearAllocator* textAllocator;
    DynamicArray<LogItem> items;
};

static LogAPIState* gState = nullptr;

void LoggerInit(AllocatorAPI* allocatorAPI, ILinearAllocator* applicationAllocator)
{
    gState = (LogAPIState*) applicationAllocator->Alloc(applicationAllocator->instance, sizeof(LogAPIState));
    // Update state pointer in api
    LogAPI* api = (LogAPI*) gAPIRegistry->Get(LOG_API_NAME);
    api->state = (void*) gState;

    gState->textAllocator = allocatorAPI->CreateLinearAllocator(Megabyte(512), Megabyte(1));
    gState->items = CreateDynamicArray<LogItem>(allocatorAPI);
}

void LoggerLog(LogType type, const char* fmt, ...)
{
    ASSERT(gState, "Logger API has not been initialized!");
    TimeAPI* timeAPI = gPlatformAPI->timeAPI;
    PlatformTime time = timeAPI->GetLocalTime();
    const u32 tempBufferSize = 32;
    char tempBuffer[tempBufferSize];
    i32 timeFormatCount = timeAPI->FormatTime(tempBuffer, tempBufferSize, time, "[hh:mm:ss] ");

    va_list varg;
    va_start(varg, fmt);
    int count = _vscprintf(fmt, varg);
    ASSERT(count > 0, "Invalid log text");
    char* text = (char*) gState->textAllocator->Alloc(gState->textAllocator->instance, (u64) timeFormatCount + count + 1);
    memcpy(text, tempBuffer, timeFormatCount);
    vsprintf(text + timeFormatCount, fmt, varg);
    va_end(varg);

    LogItem item;
    item.type = type;
    item.log = text;

    gState->items.Append(item);
}

u32 LoggerGetLogs(const LogItem** outBaseAddress)
{
    ASSERT(gState, "Logger API has not been initialized!");
    *outBaseAddress = gState->items.data;
    return gState->items.length;
}

void LoggerClearLogs()
{
    ASSERT(gState, "Logger API has not been initialized!");
    gState->textAllocator->ClearMemory(gState->textAllocator->instance);
    gState->items.Clear();
}

void LoggerShutdown(AllocatorAPI* allocatorAPI)
{
    ASSERT(gState, "Logger API has not been initialized!");
    allocatorAPI->DestroyLinearAllocator(gState->textAllocator);
    DestroyDynamicArray<LogItem>(&gState->items, allocatorAPI);
}

void RegisterLogAPI(APIRegistry* registry, bool reload)
{
    gAPIRegistry = registry;
    gPlatformAPI = (PlatformAPI*) registry->Get(PLATFORM_API_NAME);

    LogAPI logAPI = {};
    if (reload)
    {
        LogAPI* api = (LogAPI*) registry->Get(LOG_API_NAME);
        ASSERT(api, "Can't find API on reload");
        gState = (LogAPIState*) api->state;
    }

    logAPI.state = (void*) gState;
    logAPI.Init = LoggerInit;
    logAPI.Log = LoggerLog;
    logAPI.GetLogs = LoggerGetLogs;
    logAPI.Shutdown = LoggerShutdown;
    logAPI.ClearLogs = LoggerClearLogs;

    registry->Set(LOG_API_NAME, &logAPI, sizeof(LogAPI));
}
