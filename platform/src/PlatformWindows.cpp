#include "pch.h"

#include "ApiRegistry.h"
#include "Platform.h"
#include "Allocator.h"
#include "System.h"
#include "Keycode.h"

#include <Windows.h>
#include <windowsx.h>
#include <datetimeapi.h>
#include <assert.h>
#include <time.h>
#include <stdio.h>

struct WindowsPlatformData
{
    char executablePath[MAX_PATH];
    HINSTANCE hInstance;
    u64 performanceFrequency;
} gWindowsPlatformData;

struct DLLInfo
{
    HMODULE moduleHandle;
    char moduleName[100];
    char dllPath[MAX_PATH];
    char dllTempPath[MAX_PATH];
    FILETIME lastWriteTime;
};

DLLInfo pluginDlls[100];
u32 numPluginDlls = 0;

typedef void (*PluginFunction)(APIRegistry* registry, bool reload);
void LoadPlugin(APIRegistry* registry, const char* moduleName)
{
    TempAllocator tempAllocator = {};

    char* dllPath = tempAllocator.Printf("%s/%s.dll", gWindowsPlatformData.executablePath, moduleName);
    char* dllTempPath = tempAllocator.Printf("%s/%s_hotreload.dll", gWindowsPlatformData.executablePath, moduleName);

    CopyFile(dllPath, dllTempPath, FALSE);
    HMODULE module = LoadLibrary(dllTempPath);

    WIN32_FILE_ATTRIBUTE_DATA data = {};
    GetFileAttributesEx(dllPath, GetFileExInfoStandard, &data);

    DLLInfo dllInfo = {};
    dllInfo.moduleHandle = module;
    memcpy(dllInfo.moduleName, moduleName, 100);
    memcpy(dllInfo.dllPath, dllPath, MAX_PATH);
    memcpy(dllInfo.dllTempPath, dllTempPath, MAX_PATH);
    dllInfo.lastWriteTime = data.ftLastWriteTime;
    pluginDlls[numPluginDlls++] = dllInfo;

    PluginFunction Load = (PluginFunction) GetProcAddress(module, "LoadPlugin");

    Load(registry, false);
}

void HotLoadPlugins(APIRegistry* registry)
{
    for (u32 i = 0; i < numPluginDlls; ++i)
    {
        DLLInfo* info = pluginDlls + i;

        WIN32_FILE_ATTRIBUTE_DATA data = {};
        BOOL attributeResult = GetFileAttributesEx(info->dllPath, GetFileExInfoStandard, &data);

        FILETIME previousWriteTime = info->lastWriteTime;
        FILETIME newWriteTime = data.ftLastWriteTime;

        LONG result = CompareFileTime(&newWriteTime, &previousWriteTime);
        if (result == 1 && attributeResult != 0)
        {
            FreeLibrary(info->moduleHandle);

            const char* loadPath = info->dllTempPath;
            CopyFile(info->dllPath, loadPath, FALSE);
            HMODULE newModule = LoadLibrary(loadPath);

            // Load new plugin
            PluginFunction Load = (PluginFunction) GetProcAddress(newModule, "LoadPlugin");
            Load(registry, true);

            info->moduleHandle = newModule;
            info->lastWriteTime = newWriteTime;
        }
    }
}

void* WindowsVirtualMemoryAlloc(void* baseAddress, u64 size, u32 flags)
{
    DWORD allocationType = 0;
    if (flags & VA_COMMIT) {
        allocationType |= MEM_COMMIT;
    }
    if (flags & VA_RESERVE) {
        allocationType |= MEM_RESERVE;
    }
    if (flags & VA_LARGE_PAGES) {
        allocationType |= MEM_LARGE_PAGES;
    }

    printf("VirtualAlloc baseAddress:%llu, size:%lu, flags:%d\n", baseAddress, size, flags);
    void* ptr = VirtualAlloc((LPVOID) baseAddress, (SIZE_T) size, allocationType, PAGE_READWRITE);
    if (!ptr)
    {
        printf("VirtualAllocError: %d\n", GetLastError());
    }
    return ptr;
}

void WindowsVirtualMemoryFree(void* baseAddress, u64 size, u32 flags)
{
    DWORD freeType = 0;
    if (flags & VF_DECOMMIT) {
        freeType |= MEM_DECOMMIT;
    }
    if (flags & VF_RELEASE) {
        freeType |= MEM_RELEASE;
    }

    printf("VirtualFree baseAddress:%llu, size:%lu, flags:%d\n", baseAddress, size, flags);
    BOOL result = VirtualFree((LPVOID) baseAddress, (SIZE_T) size, freeType);
    if (!result)
    {
        printf("VirtualFreeError: %d\n", GetLastError());
    }
}

// Window API
#define EVENT_BUFFER_COUNT 512
struct PlatformWindow
{
    HWND windowHandle;
    WindowState state;

    InputState* inputState;
    InputEvent eventRingBuffer[EVENT_BUFFER_COUNT];
    u16 eventRingBufferHead;
    u16 eventRingBufferTail;
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
    PlatformWindow* platformWindow = (PlatformWindow*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    InputState* inputState = nullptr;
    if (platformWindow)
    {
        inputState = platformWindow->inputState;
    }

    InputEvent event;
    switch (uMsg) 
    {
        case WM_DESTROY:
        {
            platformWindow->state.isCloseRequested = true;
            PostQuitMessage(0);
        } return 0;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            event.eventType = EVENT_KEY_DOWN;
            event.controllerType = CONTROLLER_KEYBOARD;
            event.data.data0 = (u32) wParam;
            inputState->keyDown[wParam] = true;
        } break;
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            event.eventType = EVENT_KEY_UP;
            event.controllerType = CONTROLLER_KEYBOARD;
            event.data.data0 = (u32) wParam;
            inputState->keyDown[wParam] = false;
        } break;
        case WM_LBUTTONDOWN:
        {
            event.eventType = EVENT_KEY_DOWN;
            event.controllerType = CONTROLLER_MOUSE;
            event.data.data0 = (u32) MOUSE_LBUTTON;
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            inputState->mousePosClient = Vector2 {pt.x, pt.y};
            inputState->mouseKeyDown[MOUSE_LBUTTON] = true;
        } break;
        case WM_RBUTTONDOWN:
        {
            event.eventType = EVENT_KEY_DOWN;
            event.controllerType = CONTROLLER_MOUSE;
            event.data.data0 = (u32) MOUSE_RBUTTON;
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            inputState->mousePosClient = Vector2 {pt.x, pt.y};
            inputState->mouseKeyDown[MOUSE_RBUTTON] = true;
        } break;
        case WM_MBUTTONDOWN:
        {
            event.eventType = EVENT_KEY_DOWN;
            event.controllerType = CONTROLLER_MOUSE;
            event.data.data0 = (u32) MOUSE_MBUTTON;
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            inputState->mousePosClient = Vector2 {pt.x, pt.y};
            inputState->mouseKeyDown[MOUSE_MBUTTON] = true;
        } break;
        case WM_LBUTTONUP:
        {
            event.eventType = EVENT_KEY_UP;
            event.controllerType = CONTROLLER_MOUSE;
            event.data.data0 = (u32) MOUSE_LBUTTON;
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            inputState->mousePosClient = Vector2 {pt.x, pt.y};
            inputState->mouseKeyDown[MOUSE_LBUTTON] = false;
        } break;
        case WM_RBUTTONUP:
        {
            event.eventType = EVENT_KEY_UP;
            event.controllerType = CONTROLLER_MOUSE;
            event.data.data0 = (u32) MOUSE_RBUTTON;
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            inputState->mousePosClient = Vector2 {pt.x, pt.y};
            inputState->mouseKeyDown[MOUSE_RBUTTON] = false;
        } break;
        case WM_MBUTTONUP:
        {
            event.eventType = EVENT_KEY_UP;
            event.controllerType = CONTROLLER_MOUSE;
            event.data.data0 = (u32) MOUSE_MBUTTON;
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            inputState->mousePosClient = Vector2 {pt.x, pt.y};
            inputState->mouseKeyDown[MOUSE_MBUTTON] = false;
        } break;
        case WM_MOUSEMOVE:
        {
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            inputState->mousePosClient = Vector2 {pt.x, pt.y};
        } break;
        case WM_MOUSEWHEEL:
        {
            event.eventType = EVENT_MOUSEWHEEL;
            event.controllerType = CONTROLLER_MOUSE;
            event.data.data1 = (i32) GET_WHEEL_DELTA_WPARAM(wParam);;
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            inputState->mousePosClient = Vector2 {pt.x, pt.y};
        } break;
        case WM_CHAR:
        {
            event.eventType = EVENT_CHARACTER_TYPED;
            event.controllerType = CONTROLLER_KEYBOARD;
            event.data.data0 = (u32) wParam;
        } break;
        case WM_SIZE:
        {
            //u32 width = LOWORD(lParam);
            //u32 height = HIWORD(lParam);

        } return 0;
        default:
        {

        } return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    platformWindow->eventRingBuffer[platformWindow->eventRingBufferTail] = event;
    platformWindow->eventRingBufferTail = (platformWindow->eventRingBufferTail + 1) % EVENT_BUFFER_COUNT;
    ASSERT(platformWindow->eventRingBufferTail != platformWindow->eventRingBufferHead, "Event ring buffer overflow");

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
};

PlatformWindow* WindowsCreateWindow(ILinearAllocator* allocator, const char* name, WindowSize size)
{
    HINSTANCE hInstance = gWindowsPlatformData.hInstance;

    const char* windowClassName = "WindowClass";
    WNDCLASS windowClass = {};
    windowClass.lpfnWndProc = &WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = windowClassName;

    RegisterClass(&windowClass);

    HWND windowHandle = CreateWindowEx(0,
        windowClassName,
        name,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        size.clientWidth,
        size.clientHeight,
        NULL,
        NULL,
        hInstance,
        nullptr);

    PlatformWindow* platformWindow = (PlatformWindow*) allocator->Alloc(allocator->instance, sizeof(PlatformWindow));
    platformWindow->windowHandle = windowHandle;
    platformWindow->inputState = (InputState*) allocator->Alloc(allocator->instance, sizeof(InputState));

    SetWindowLongPtr(windowHandle, GWLP_USERDATA, (LONG_PTR)platformWindow);
    return platformWindow;
}

WindowState WindowsProcessMessages(PlatformWindow* window)
{
    MSG msg;
    while (PeekMessage(&msg, window->windowHandle, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return window->state;
}

void* WindowsGetWindowHandle(PlatformWindow* window)
{
    return (void*) window->windowHandle;
}

void WindowsShowWindow(PlatformWindow* window)
{
    ShowWindow(window->windowHandle, SW_SHOWDEFAULT);
}

WindowSize WindowsGetClientSize(PlatformWindow* window)
{
    RECT clientRect = {};
    GetClientRect(window->windowHandle, &clientRect);

    WindowSize size = {};
    size.clientWidth = clientRect.right - clientRect.left;
    size.clientHeight = clientRect.bottom - clientRect.top;
    return size;
}

// Input
u32 WindowsPullEvents(PlatformWindow* window, ILinearAllocator* allocator, InputEvent** outBaseAddress)
{
    i32 sub = window->eventRingBufferTail - window->eventRingBufferHead;
    u32 eventCount = (u32) abs(sub);

    if (sub > 0)
    {
        InputEvent* dest = (InputEvent*) allocator->Alloc(allocator->instance, eventCount * sizeof(InputEvent));
        memcpy(dest, window->eventRingBuffer + window->eventRingBufferHead, eventCount * sizeof(InputEvent));
        *outBaseAddress = dest;
    }
    else if (sub < 0)
    {
        InputEvent* dest = (InputEvent*) allocator->Alloc(allocator->instance, eventCount * sizeof(InputEvent));
        u32 headToEnd = EVENT_BUFFER_COUNT - window->eventRingBufferHead;
        memcpy(dest, window->eventRingBuffer + window->eventRingBufferHead, headToEnd * sizeof(InputEvent));
        u32 startToTail = window->eventRingBufferTail;
        memcpy(dest + headToEnd, window->eventRingBuffer, startToTail * sizeof(InputEvent));
        *outBaseAddress = dest;
    }

    window->eventRingBufferHead = window->eventRingBufferTail;
    return eventCount;
}

// Date-time
inline u64 ConvertSystemTimeToU64(const SYSTEMTIME* systemTime)
{
    FILETIME fileTime;
    SystemTimeToFileTime(systemTime, &fileTime);
    ULARGE_INTEGER largeInteger;
    largeInteger.LowPart = fileTime.dwLowDateTime;
    largeInteger.HighPart = fileTime.dwHighDateTime;

    return (u64) largeInteger.QuadPart;
}

inline void ConvertU64ToSystemTime(u64 time, SYSTEMTIME* outSystemTime)
{
    ULARGE_INTEGER largeInteger;
    largeInteger.QuadPart = time;

    FILETIME fileTime;
    fileTime.dwLowDateTime = largeInteger.LowPart;
    fileTime.dwHighDateTime = largeInteger.HighPart;

    FileTimeToSystemTime(&fileTime, outSystemTime);
}

PlatformTime WindowsGetLocalTime()
{
    SYSTEMTIME systemTime;
    GetLocalTime(&systemTime);
    u64 time = ConvertSystemTimeToU64(&systemTime);

    return PlatformTime{ time };
}

i32 WindowsFormatTime(char* outBuffer, u32 outBufferSize, PlatformTime time, const char* format)
{
    SYSTEMTIME systemTime;
    ConvertU64ToSystemTime(time.time, &systemTime);
    return GetTimeFormat(LOCALE_NAME_USER_DEFAULT, TIME_FORCE24HOURFORMAT, &systemTime, format, outBuffer, outBufferSize) - 1;
}

InputState* WindowsGetInputState(PlatformWindow* window)
{
    return window->inputState;
}

u64 WindowsPerformanceCounterNanoseconds()
{
    LARGE_INTEGER integer;
    QueryPerformanceCounter(&integer);

    integer.QuadPart *= 1000000000;
    return (u64) integer.QuadPart / gWindowsPlatformData.performanceFrequency;
}

int main(int argc, char* argv[])
{
    HINSTANCE hInstance = GetModuleHandle(0);
    SetProcessDPIAware();
    gWindowsPlatformData.hInstance = hInstance;

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    gWindowsPlatformData.performanceFrequency = (u64) frequency.QuadPart;

    size_t len = strlen(argv[0]);
    memcpy(gWindowsPlatformData.executablePath, argv[0], len + 1);
    PathRemoveFilename(gWindowsPlatformData.executablePath, true);

    VirtualMemoryAPI virtualMemoryAPI = {};
    virtualMemoryAPI.Alloc = &WindowsVirtualMemoryAlloc;
    virtualMemoryAPI.Free = &WindowsVirtualMemoryFree;

    WindowAPI windowAPI = {};
    windowAPI.CreatePlatformWindow = WindowsCreateWindow;
    windowAPI.ShowPlatformWindow = WindowsShowWindow;
    windowAPI.GetPlatformWindowClientSize = WindowsGetClientSize;
    windowAPI.GetPlatformWindowHandle = WindowsGetWindowHandle;
    windowAPI.ProcessMessages = WindowsProcessMessages;

    InputAPI inputAPI = {};
    inputAPI.PullEvents = WindowsPullEvents;
    inputAPI.GetState = WindowsGetInputState;

    TimeAPI timeAPI = {};
    timeAPI.FormatTime = WindowsFormatTime;
    timeAPI.GetLocalTime = WindowsGetLocalTime;
    timeAPI.GetPerformanceCounterTimeNanoseconds = WindowsPerformanceCounterNanoseconds;

    AllocatorAPI allocatorAPI = CreateAllocatorAPI(&virtualMemoryAPI);
    ILinearAllocator* applicationAllocator = allocatorAPI.CreateLinearAllocator(Megabyte(100), Megabyte(1));

    APIRegistry registry = CreateApiRegistry(&allocatorAPI, applicationAllocator);

    PlatformAPI platformAPI = {};
    platformAPI.virtualMemoryAPI = &virtualMemoryAPI;
    platformAPI.windowAPI = &windowAPI;
    platformAPI.inputAPI = &inputAPI;
    platformAPI.LoadPlugin = &LoadPlugin;
    platformAPI.timeAPI = &timeAPI;

    registry.Set(PLATFORM_API_NAME, &platformAPI, sizeof(PlatformAPI));
    registry.Set(ALLOCATOR_API_NAME, &allocatorAPI, sizeof(AllocatorAPI));

    LoadPlugin(&registry, "Foundation");
    LoadPlugin(&registry, "System");
    SystemAPI* systemAPI = (SystemAPI*) registry.Get(SYSTEM_API_NAME);
    
    systemAPI->Init(applicationAllocator);

    bool quitRequested = false;
    while (!quitRequested)
    {
        HotLoadPlugins(&registry);
        quitRequested = !systemAPI->Update();
    }

    systemAPI->Shutdown();

    return 0;
}


