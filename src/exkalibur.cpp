#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstdarg>

#define EX_ASSERT(x) \
    if (!(x)) { MessageBoxA(0, #x, "Assertion Failure", MB_OK); __debugbreak(); }

void
EXPrint(const char *format, ...) {
    static char buffer[1024];
    va_list arg_list;
    va_start(arg_list, format);
    vsprintf_s(buffer, sizeof(buffer), format, arg_list);
    va_end(arg_list);
    printf(buffer);
}

enum {
    EX_MAX_KEYS = 256,
    EX_MAX_TEXT = 256,
    EX_MAX_ERROR = 1024,
    EX_FALSE = 0,
    EX_TRUE = 1,
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
    float threshhold;
    float value;
    EXBOOL down;
    EXBOOL pressed;
    EXBOOL released;
};

struct EXAXIS {
    float value;
    float threshhold;
};

struct EXSTICK {
    EXAXIS x;
    EXAXIS y;
};

struct EXGAMEPAD {
    EXDIGITALBUTTON a_button;
    EXDIGITALBUTTON b_button;
    EXDIGITALBUTTON x_button;
    EXDIGITALBUTTON y_button;
    EXANALOGBUTTON left_trigger;
    EXANALOGBUTTON right_trigger;
    EXDIGITALBUTTON left_bumper_button;
    EXDIGITALBUTTON right_bumper_button;
    EXDIGITALBUTTON up_button;
    EXDIGITALBUTTON down_button;
    EXDIGITALBUTTON left_button;
    EXDIGITALBUTTON right_button;
    EXSTICK left_thumb_stick;
    EXSTICK right_thumb_stick;
    EXDIGITALBUTTON left_thumb_button;
    EXDIGITALBUTTON right_thumb_button;
    EXDIGITALBUTTON select_button;
    EXDIGITALBUTTON start_button;
};

struct EXMOUSE {
    EXDIGITALBUTTON left_button;
    EXDIGITALBUTTON right_button;
    int wheel;
    int delta_wheel;
    EXINT2 position;
    EXINT2 delta_position;
    EXINT2 screen_position;
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
};

struct EXWIN32 {
    HWND window;
    void *main_fiber;
    void *message_fiber;
};

struct EXKALIBUR {
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
    char text_buffer[EX_MAX_TEXT];

    EXWIN32 win32;
};

LRESULT CALLBACK
EXWindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;
    EXKALIBUR *state = (EXKALIBUR*)GetWindowLongPtrA(window, GWLP_USERDATA);
    switch (message) {
    case WM_DESTROY: {
        state->quit = EX_TRUE;
    } break;
    case WM_TIMER: {
        SwitchToFiber(state->win32.main_fiber);
    } break;
    case WM_ENTERMENULOOP:
    case WM_ENTERSIZEMOVE: {
        SetTimer(window, 0, 1, 0);
    } break;
    case WM_EXITMENULOOP:
    case WM_EXITSIZEMOVE: {
        KillTimer(window, 0);
    } break;
    default:
        result = DefWindowProc(window, message, wparam, lparam);
    }
    return result;
}

void CALLBACK
EXMessageFiberProc(EXKALIBUR *state) {
    for (;;) {
        MSG message;
        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
        SwitchToFiber(state->win32.main_fiber);
    }
}

void
EXPullWindow(EXKALIBUR *state) {
    RECT window_rectangle;
    GetWindowRect(state->win32.window, &window_rectangle);
    
    RECT client_rectangle;
    GetClientRect(state->win32.window, &client_rectangle);

    state->window.position.x = window_rectangle.left + client_rectangle.left;
    state->window.position.y = window_rectangle.top + client_rectangle.top;
    state->window.size.x = client_rectangle.right - client_rectangle.left;
    state->window.size.y = client_rectangle.bottom - client_rectangle.top;
}

void
EXPullTime(EXKALIBUR *state) {
    LARGE_INTEGER large_integer;
    QueryPerformanceCounter(&large_integer);
    uint64_t current_ticks = large_integer.QuadPart;
    
    state->time.delta_ticks = current_ticks - state->time.ticks - state->time.initial_ticks;
    state->time.ticks = current_ticks - state->time.initial_ticks;

    state->time.delta_nanoseconds = (1000 * 1000 * 1000 * state->time.delta_ticks) / state->time.ticks_per_second;
    state->time.delta_microseconds = state->time.delta_nanoseconds / 1000;
    state->time.delta_milliseconds = state->time.delta_microseconds / 1000;
    state->time.delta_seconds = (float)state->time.delta_ticks / (float)state->time.ticks_per_second;
    
    state->time.nanoseconds = (1000 * 1000 * 1000 * state->time.ticks) / state->time.ticks_per_second;
    state->time.microseconds = state->time.nanoseconds / 1000;
    state->time.milliseconds = state->time.microseconds / 1000;
    state->time.seconds = (double)state->time.ticks / (double)state->time.ticks_per_second;
}

void
EXExitWithError(EXKALIBUR *state) {
    MessageBoxA(0, state->error, "EXKALIBUR ERROR", MB_ICONEXCLAMATION | MB_OK);
    ExitProcess(0);
}

void
EXPull(EXKALIBUR *state) {
    if (!state->initialized) {
        if (!state->error) state->error = "EXKALIBUR was not initialized.";
        EXExitWithError(state);
        return;
    }
    SwitchToFiber(state->win32.message_fiber);
    EXPullWindow(state);
    EXPullTime(state);
}

void
EXPush(EXKALIBUR *state) {
}

void
EXUpdate(EXKALIBUR *state) {
    EXPush(state);
    EXPull(state);
}

EXBOOL
EXInitialize(EXKALIBUR *state) {
    if (!state->window.title) state->window.title = "EXKALIBUR";

    int window_x;
    if (state->window.position.x) window_x = state->window.position.x;
    else window_x = CW_USEDEFAULT;
    
    int window_y;
    if (state->window.position.y) window_y = state->window.position.y;
    else window_y = CW_USEDEFAULT;
    
    int window_width;
    if (state->window.size.x) window_width = state->window.size.x;
    else window_width = CW_USEDEFAULT;
    
    int window_height;
    if (state->window.size.y) window_height = state->window.size.y;
    else window_height = CW_USEDEFAULT;

    if (window_width != CW_USEDEFAULT && window_height != CW_USEDEFAULT) {
        RECT window_rectangle;
        window_rectangle.left = 0;
        window_rectangle.right = window_width;
        window_rectangle.top = 0;
        window_rectangle.bottom = window_height;
        if (AdjustWindowRect(&window_rectangle, WS_OVERLAPPEDWINDOW, 0)) {
            window_width = window_rectangle.right - window_rectangle.left;
            window_height = window_rectangle.bottom - window_rectangle.top;
        }
    }

    state->win32.main_fiber = ConvertThreadToFiber(0);
    EX_ASSERT(state->win32.main_fiber);
    state->win32.message_fiber = CreateFiber(0, (PFIBER_START_ROUTINE)EXMessageFiberProc, state);
    EX_ASSERT(state->win32.message_fiber);
    
    WNDCLASSA window_class = {};
    window_class.lpfnWndProc = EXWindowProc;
    window_class.lpszClassName = "EXKALIBUR";
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    if (RegisterClassA(&window_class) == 0) {
        state->error = "Failed to register window class.";
        return EX_FALSE;
    }

    state->win32.window = CreateWindow("EXKALIBUR", state->window.title, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                       window_x, window_y, window_width, window_height,
                                       0, 0, 0, 0);
    if (!state->win32.window) {
        state->error = "Failed to create window.";
        return EX_FALSE;
    }

    SetWindowLongPtr(state->win32.window, GWLP_USERDATA, (LONG_PTR)state);

    LARGE_INTEGER large_integer;
    QueryPerformanceFrequency(&large_integer);
    state->time.ticks_per_second = large_integer.QuadPart;

    state->initialized = EX_TRUE;
    EXPull(state);
    return EX_TRUE;
}

EXKALIBUR exk;

int main() {
    exk.window.size.x = 640;
    exk.window.size.y = 480;
    EXInitialize(&exk);
    while (!exk.quit) {
        // EXPull(&exk);
        static double last_print_time = 0.0f;
        if (exk.time.seconds - last_print_time > 0.1f) {
            EXPrint("x: %d y: %d, dx: %d dy: %d\n", exk.window.position.x, exk.window.position.y, exk.window.size.x, exk.window.size.y);
            EXPrint("delta_ticks: %llu, time_ticks: %llu\n", exk.time.delta_ticks, exk.time.ticks);
            EXPrint("ns: %llu, us: %llu, ms: %llu\n", exk.time.nanoseconds, exk.time.microseconds, exk.time.milliseconds);
            EXPrint("ds: %f, dns: %llu, dus: %llu, dms: %llu\n", exk.time.delta_seconds, exk.time.delta_nanoseconds, exk.time.delta_microseconds, exk.time.delta_milliseconds);
            last_print_time = exk.time.seconds;
        }
        EXUpdate(&exk);
    }
    return 0;
}
