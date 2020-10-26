#pragma once

#define IMGUI_API_NAME "IMGUI"

struct ILinearAllocator;
struct ImGuiContext;
struct PlatformWindow;
struct InputEvent;

struct ImguiAPI
{
    void* state;

    void (*Init)(ILinearAllocator* applicationAllocator, PlatformWindow* window);
    void (*BeginFrame)(PlatformWindow* window, InputEvent* events, u32 eventSize);
    void (*Render)();
    void (*Shutdown)();

    bool (*Begin)(const char* name, bool* p_open, ImGuiWindowFlags flags);
    void (*End)();
    bool (*Button)(const char* label, const ImVec2& size);
    bool (*Combo)(const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items);
    bool (*Checkbox)(const char* label, bool* v);
    bool (*BeginPopup)(const char* str_id, ImGuiWindowFlags flags);
    void (*EndPopup)();
    void (*OpenPopup)(const char* str_id, ImGuiPopupFlags popup_flags);
    void (*SameLine)(float offset_from_start_x, float spacing);
    void (*Separator)();
    bool (*BeginChild)(const char* str_id, const ImVec2& size, bool border, ImGuiWindowFlags flags);
    void (*EndChild)();
    void (*PushStyleVar)(ImGuiStyleVar idx, const ImVec2& val);
    void (*PopStyleVar)(int count);

    void (*TextUnformatted)(const char* text, const char* text_end);
    void (*Text)(const char* fmt, ...);
    void (*TextColored)(const ImVec4& col, const char* fmt, ...);

    bool (*TreeNode)(const char* str_id, const char* fmt, ...);
    void (*TreePop)();

    float (*GetScrollX)();
    float (*GetScrollY)();
    float (*GetScrollMaxX)();
    float (*GetScrollMaxY)();
    void (*SetScrollX)(float scroll_x);
    void (*SetScrollY)(float scroll_y);
    void (*SetScrollHereX)(float center_x_ratio);
    void (*SetScrollHereY)(float center_y_ratio);

};
