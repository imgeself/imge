#pragma once

struct AllocatorAPI;
struct ILinearAllocator;
struct APIRegistry;

enum LogType
{
    LOG_DEBUG,
    LOG_WARNING,
    LOG_ERROR
};

struct LogItem
{
    LogType type;
    const char* log;
};

#define LOG_API_NAME "LogAPI"

struct LogAPI
{
    void* state;

    void (*Init)(AllocatorAPI* allocatorAPI, ILinearAllocator* applicationAllocator);
    void (*Log)(LogType type, const char* fmt, ...);
    u32 (*GetLogs)(const LogItem** outBaseAddress);
    void (*ClearLogs)();
    void (*Shutdown)(AllocatorAPI* allocatorAPI);
};

#define LOG_DEBUG(logAPI, ...) logAPI->Log(LOG_DEBUG, __VA_ARGS__)
#define LOG_WARNING(logAPI, ...) logAPI->Log(LOG_WARNING, __VA_ARGS__)
#define LOG_ERROR(logAPI, ...) logAPI->Log(LOG_ERROR, __VA_ARGS__)