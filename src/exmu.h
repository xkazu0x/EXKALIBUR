#pragma once

#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

enum {
    EX_FALSE = 0,
    EX_TRUE = 1,
    EX_MAX_KEYS = 256,
    EX_MAX_TEXT = 256,
    EX_MAX_ERROR = 1024,
    EX_MAX_WARN = 1024,
    EX_KEY_CONTROL = 0x11,
    EX_KEY_ESCAPE = 0x1B,
};

typedef uint8_t EXBOOL;

struct EXINT2 {
    int x;
    int y;
};

struct EXFLOAT2 {
    float x;
    float y;
};

struct EXDIGITALBUTTON {
    EXBOOL down;
    EXBOOL pressed;
    EXBOOL released;
};

struct EXANALOGBUTTON {
    float threshold;
    float value;
    EXBOOL down;
    EXBOOL pressed;
    EXBOOL released;
};

struct EXSTICK {
    float threshold;
    float x;
    float y;
};

struct EXGAMEPAD {
    EXBOOL connected;
    EXDIGITALBUTTON up_button;
    EXDIGITALBUTTON down_button;
    EXDIGITALBUTTON left_button;
    EXDIGITALBUTTON right_button;
    EXDIGITALBUTTON start_button;
    EXDIGITALBUTTON back_button;
    EXDIGITALBUTTON left_thumb_button;
    EXDIGITALBUTTON right_thumb_button;
    EXDIGITALBUTTON left_shoulder_button;
    EXDIGITALBUTTON right_shoulder_button;
    EXDIGITALBUTTON a_button;
    EXDIGITALBUTTON b_button;
    EXDIGITALBUTTON x_button;
    EXDIGITALBUTTON y_button;
    EXANALOGBUTTON left_trigger;
    EXANALOGBUTTON right_trigger;
    EXSTICK left_thumb_stick;
    EXSTICK right_thumb_stick;
    float left_motor_speed;
    float right_motor_speed;
};

struct EXMOUSE {
    EXDIGITALBUTTON left_button;
    EXDIGITALBUTTON right_button;
    int wheel;
    int delta_wheel;
    EXINT2 position;
    EXINT2 delta_position;
};

struct EXKEYBOARD {
    EXDIGITALBUTTON keys[EX_MAX_KEYS];
};

struct EXTIME {
    float delta_seconds;
    uint64_t delta_ticks;
    uint64_t delta_nanoseconds;
    uint64_t delta_microseconds;
    uint64_t delta_milliseconds;

    double seconds;
    uint64_t ticks;
    uint64_t nanoseconds;
    uint64_t microseconds;
    uint64_t milliseconds;

    uint64_t initial_ticks;
    uint64_t ticks_per_second;
};
    
struct EXWINDOW {
    const char *title;
    EXINT2 position;
    EXINT2 size;
    EXBOOL resized;
    EXBOOL centered;
};

typedef void *HANDLE;

typedef struct _XINPUT_STATE XINPUT_STATE;
typedef struct _XINPUT_VIBRATION XINPUT_VIBRATION;
#define X_INPUT_GET_STATE(name) unsigned long __stdcall name(unsigned long dwUseIndex, XINPUT_STATE *pState)
#define X_INPUT_SET_STATE(name) unsigned long __stdcall name(unsigned long dwUseIndex, XINPUT_VIBRATION *pVibration)

typedef X_INPUT_GET_STATE(XINPUTGETSTATE);
typedef X_INPUT_SET_STATE(XINPUTSETSTATE);

struct EXWIN32 {
    HANDLE window;
    HANDLE device_context;
    
    void *main_fiber;
    void *message_fiber;
    
    XINPUTGETSTATE *xinput_get_state;
    XINPUTSETSTATE *xinput_set_state;

    HANDLE wgl_context;
};

struct EXMU {
    EXBOOL initialized;
    EXBOOL quit;

    const char *error;
    char error_buffer[EX_MAX_ERROR];
    
    EXWINDOW window;
    EXTIME time;
    EXKEYBOARD keyboard;
    EXGAMEPAD gamepad;
    EXMOUSE mouse;

    const char *text;
    char *text_end;
    char text_buffer[EX_MAX_TEXT];
    
    EXWIN32 win32;
};

EXBOOL EXMU_initialize(EXMU *state);
EXBOOL EXMU_pull(EXMU *state);
EXBOOL EXMU_push(EXMU *state);
