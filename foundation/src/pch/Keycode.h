#pragma once

#ifdef PLATFORM_WINDOWS
#include <Windows.h>

#define MOUSE_LBUTTON 0
#define MOUSE_RBUTTON 1
#define MOUSE_MBUTTON 2

#define KEY_UNKNOWN            -1

#define KEY_NP_0               VK_NUMPAD0
#define KEY_NP_1               VK_NUMPAD1
#define KEY_NP_2               VK_NUMPAD2
#define KEY_NP_3               VK_NUMPAD3
#define KEY_NP_4               VK_NUMPAD4
#define KEY_NP_5               VK_NUMPAD5
#define KEY_NP_6               VK_NUMPAD6
#define KEY_NP_7               VK_NUMPAD7
#define KEY_NP_8               VK_NUMPAD8
#define KEY_NP_9               VK_NUMPAD9

#define KEY_0                  48
#define KEY_1                  49
#define KEY_2                  50
#define KEY_3                  51
#define KEY_4                  52
#define KEY_5                  53
#define KEY_6                  54
#define KEY_7                  55
#define KEY_8                  56
#define KEY_9                  57

#define KEY_A                  65
#define KEY_B                  66
#define KEY_C                  67
#define KEY_D                  68
#define KEY_E                  69
#define KEY_F                  70
#define KEY_G                  71
#define KEY_H                  72
#define KEY_I                  73
#define KEY_J                  74
#define KEY_K                  75
#define KEY_L                  76
#define KEY_M                  77
#define KEY_N                  78
#define KEY_O                  79
#define KEY_P                  80
#define KEY_Q                  81
#define KEY_R                  82
#define KEY_S                  83
#define KEY_T                  84
#define KEY_U                  85
#define KEY_V                  86
#define KEY_W                  87
#define KEY_X                  88
#define KEY_Y                  89
#define KEY_Z                  90

#define KEY_SEMICOLON          VK_OEM_1       /* ; */
#define KEY_SLASH              VK_OEM_2       /* / */
#define KEY_APOSTROPHE         VK_OEM_3       /* ` */
#define KEY_LEFT_BRACKET       VK_OEM_4       /* [ */
#define KEY_BACKSLASH          VK_OEM_5       /* \ */
#define KEY_RIGHT_BRACKET      VK_OEM_6       /* ] */
#define KEY_QUOTE              VK_OEM_7       /* ' */
#define KEY_COMMA              VK_OEM_COMMA   /* , */
#define KEY_MINUS              VK_OEM_MINUS   /* - */
#define KEY_PERIOD             VK_OEM_PERIOD  /* . */
#define KEY_EQUAL              VK_OEM_PLUS    /* = */

#define KEY_SPACE              VK_SPACE
#define KEY_ESCAPE             VK_ESCAPE
#define KEY_ENTER              VK_RETURN
#define KEY_TAB                VK_TAB
#define KEY_BACKSPACE          VK_BACK
#define KEY_INSERT             VK_INSERT
#define KEY_DELETE             VK_DELETE
#define KEY_RIGHT              VK_RIGHT
#define KEY_LEFT               VK_LEFT
#define KEY_DOWN               VK_DOWN
#define KEY_UP                 VK_UP
#define KEY_PAGE_UP            VK_PRIOR
#define KEY_PAGE_DOWN          VK_NEXT
#define KEY_HOME               VK_HOME
#define KEY_END                VK_END
#define KEY_CAPS_LOCK          VK_CAPITAL
#define KEY_NUM_LOCK           VK_NUMLOCK
#define KEY_PAUSE              VK_PAUSE

#define KEY_F1                 VK_F1
#define KEY_F2                 VK_F2
#define KEY_F3                 VK_F3
#define KEY_F4                 VK_F4
#define KEY_F5                 VK_F5
#define KEY_F6                 VK_F6
#define KEY_F7                 VK_F7
#define KEY_F8                 VK_F8
#define KEY_F9                 VK_F9
#define KEY_F10                VK_F10
#define KEY_F11                VK_F11
#define KEY_F12                VK_F12
#define KEY_F13                VK_F13
#define KEY_F14                VK_F14
#define KEY_F15                VK_F15
#define KEY_F16                VK_F16
#define KEY_F17                VK_F17
#define KEY_F18                VK_F18
#define KEY_F19                VK_F19
#define KEY_F20                VK_F20
#define KEY_F21                VK_F21
#define KEY_F22                VK_F22
#define KEY_F23                VK_F23
#define KEY_F24                VK_F24

#define KEY_LEFT_SHIFT         VK_LSHIFT
#define KEY_LEFT_CONTROL       VK_LCONTROL
#define KEY_LEFT_ALT           VK_LMENU
#define KEY_LEFT_SYSTEM        VK_LWIN
#define KEY_RIGHT_SHIFT        VK_RSHIFT
#define KEY_RIGHT_CONTROL      VK_RCONTROL
#define KEY_RIGHT_ALT          VK_RMENU
#define KEY_RIGHT_SYSTEM       VK_RWIN

#endif