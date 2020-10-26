#pragma once

#define PLATFORM_API_NAME "PlatformAPI"

struct APIRegistry;
struct ILinearAllocator;

// Virtual Memory
enum VirtualAllocationFlag
{
    VA_RESERVE  = BIT(0),
    VA_COMMIT   = BIT(1),

    VA_LARGE_PAGES = BIT(2)
};

enum VirtualFreeFlag
{
    VF_DECOMMIT  = BIT(0),
    VF_RELEASE   = BIT(1)
};

struct VirtualMemoryAPI
{
    void* (*Alloc)(void* baseAddress, u64 size, u32 flags);
    void (*Free)(void* baseAddress, u64 size, u32 flags);
};

// Input
enum InputEventType
{
    EVENT_KEY_DOWN,
    EVENT_KEY_UP,
    EVENT_MOUSEWHEEL,
    EVENT_CHARACTER_TYPED,
};

enum InputControllerType
{
    CONTROLLER_KEYBOARD,
    CONTROLLER_MOUSE
};

// Generic data for input
struct InputData
{
    u32 data0;
    i32 data1;
    i32 data2;
    i32 data3;
};

struct InputEvent
{
    InputControllerType controllerType;
    InputEventType eventType;
    InputData data;
};

struct Vector2
{
    i64 x;
    i64 y;
};

struct InputState
{
    Vector2 mousePosClient;

    bool keyDown[512];
    bool mouseKeyDown[4];
};

struct PlatformWindow;

struct InputAPI
{
    // Get input events from specified window using the allocator, returns event count
    u32 (*PullEvents)(PlatformWindow* window, ILinearAllocator* allocator, InputEvent** outBaseAddress);
    InputState* (*GetState)(PlatformWindow* window);
};

// Windowing
struct WindowSize
{
    u32 clientWidth;
    u32 clientHeight;
};

struct WindowState
{
    bool isCloseRequested;
};

struct WindowAPI
{
    PlatformWindow* (*CreatePlatformWindow)(ILinearAllocator* allocator, const char* name, WindowSize size);
    void (*ShowPlatformWindow)(PlatformWindow* window);
    void* (*GetPlatformWindowHandle)(PlatformWindow* window);
    WindowSize (*GetPlatformWindowClientSize)(PlatformWindow* window);

    WindowState (*ProcessMessages)(PlatformWindow* window);
};

// Date time API
struct PlatformTime
{
    u64 time;
};

struct TimeAPI
{
    PlatformTime (*GetLocalTime)();
    i32 (*FormatTime)(char* outBuffer, u32 outBufferSize, PlatformTime time, const char* format);

    u64 (*GetPerformanceCounterTimeNanoseconds)();
};

struct PlatformAPI
{
    VirtualMemoryAPI* virtualMemoryAPI;
    WindowAPI* windowAPI;
    InputAPI* inputAPI;
    TimeAPI* timeAPI;

    void (*LoadPlugin)(APIRegistry* registry, const char* moduleName);
};
