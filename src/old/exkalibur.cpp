#include <windows.h>
#include <xinput.h>

#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#ifdef _DEBUG
#define EX_ASSERT(x) \
    if (!(x)) { MessageBoxA(0, #x, "Assertion Failure", MB_OK); __debugbreak(); }
#else
#define EX_ASSERT(x)
#endif

enum {
    EX_FALSE = 0,
    EX_TRUE = 1,
    EX_MAX_KEYS = 256,
    EX_MAX_TEXT = 256,
    EX_MAX_ERROR = 1024,
    EX_MAX_WARN = 1024,
    EX_KEY_CONTROL = VK_CONTROL,
    EX_KEY_ESCAPE = VK_ESCAPE,
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
};

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUseIndex, XINPUT_STATE *pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUseIndex, XINPUT_VIBRATION *pVibration)

typedef X_INPUT_GET_STATE(XINPUTGETSTATE);
typedef X_INPUT_SET_STATE(XINPUTSETSTATE);

X_INPUT_GET_STATE(xinput_get_state_) { return 0; }
X_INPUT_SET_STATE(xinput_set_state_) { return 0; }

struct EXWIN32 {
    HWND window;
    void *main_fiber;
    void *message_fiber;
    XINPUTGETSTATE *xinput_get_state;
    XINPUTSETSTATE *xinput_set_state;
};

struct EXMU {
    EXBOOL initialized;
    EXBOOL quit;

    const char *error;
    char error_buffer[EX_MAX_ERROR];

    const char *warn;
    char warn_buffer[EX_MAX_WARN];
    
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

void
EXPullWindow(EXMU *state) {
    state->window.resized = EX_FALSE;
    
    state->text_end = state->text_buffer;
    state->text = 0;
    
    state->mouse.delta_position.x = 0;
    state->mouse.delta_position.y = 0;
    state->mouse.delta_wheel = 0;
    state->mouse.left_button.pressed = EX_FALSE;
    state->mouse.left_button.released = EX_FALSE;
    state->mouse.right_button.pressed = EX_FALSE;
    state->mouse.right_button.released = EX_FALSE;
    
    SwitchToFiber(state->win32.message_fiber);

    state->mouse.position.x += state->mouse.delta_position.x;
    state->mouse.position.y += state->mouse.delta_position.y;
    state->mouse.wheel += state->mouse.delta_wheel;
    
    if (state->text_end != state->text_buffer) {
        state->text = state->text_buffer;
    }
    
    RECT client_rectangle;
    GetClientRect(state->win32.window, &client_rectangle);
    
    state->window.size.x = client_rectangle.right - client_rectangle.left;
    state->window.size.y = client_rectangle.bottom - client_rectangle.top;

    POINT window_position = { client_rectangle.left, client_rectangle.top };
    ClientToScreen(state->win32.window, &window_position);
    
    state->window.position.x = window_position.x;
    state->window.position.y = window_position.y;
}

void
EXPullTime(EXMU *state) {
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
EXPullDigitalButton(EXDIGITALBUTTON *button, EXBOOL down) {
    EXBOOL was_down = button->down;
    button->pressed = !was_down && down;
    button->released = was_down && !down;
    button->down = down;
}

void
EXPullAnalogButton(EXANALOGBUTTON *button, float value) {
    button->value = value;
    EXBOOL was_down = button->down;
    button->down = (value >= button->threshold);
    button->pressed = !was_down && button->down;
    button->released = was_down && !button->down;
}

void
EXPullStick(EXSTICK *stick, float x, float y) {    
    if (fabs(x) <= stick->threshold) x = 0.0f;
    stick->x = x;
    
    if (fabs(y) <= stick->threshold) y = 0.0f;
    stick->y = y;
}

void
EXPullKeys(EXMU *state) {
    BYTE keyboard_state[EX_MAX_KEYS];
    if (!GetKeyboardState(keyboard_state)) return;
    for (int key = 0; key < EX_MAX_KEYS; key++) {
        EXPullDigitalButton(state->keyboard.keys + key, keyboard_state[key] >> 7);
    }
}

void
EXPullMouse(EXMU *state) {
    POINT mouse_position;
    GetCursorPos(&mouse_position);
    
    mouse_position.x -= state->window.position.x;
    mouse_position.y -= state->window.position.y;

    state->mouse.position.x = mouse_position.x;
    state->mouse.position.y = mouse_position.y;
}
    
void
EXPullGamepad(EXMU *state) {
    if (!state->win32.xinput_get_state) return;

    XINPUT_STATE xinput_state = {};
    if (state->win32.xinput_get_state(0, &xinput_state) == ERROR_DEVICE_NOT_CONNECTED) {
        state->gamepad.connected = EX_FALSE;
        return;
    }

    state->gamepad.connected = EX_TRUE;
    
    EXPullDigitalButton(&state->gamepad.up_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) == XINPUT_GAMEPAD_DPAD_UP);
    EXPullDigitalButton(&state->gamepad.down_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) == XINPUT_GAMEPAD_DPAD_DOWN);
    EXPullDigitalButton(&state->gamepad.left_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) == XINPUT_GAMEPAD_DPAD_LEFT);
    EXPullDigitalButton(&state->gamepad.right_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) == XINPUT_GAMEPAD_DPAD_RIGHT);
    EXPullDigitalButton(&state->gamepad.start_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_START) == XINPUT_GAMEPAD_START);
    EXPullDigitalButton(&state->gamepad.back_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) == XINPUT_GAMEPAD_BACK);
    EXPullDigitalButton(&state->gamepad.left_thumb_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) == XINPUT_GAMEPAD_LEFT_THUMB);
    EXPullDigitalButton(&state->gamepad.right_thumb_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) == XINPUT_GAMEPAD_RIGHT_THUMB);
    EXPullDigitalButton(&state->gamepad.left_shoulder_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) == XINPUT_GAMEPAD_LEFT_SHOULDER);
    EXPullDigitalButton(&state->gamepad.right_shoulder_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) == XINPUT_GAMEPAD_RIGHT_SHOULDER);
    EXPullDigitalButton(&state->gamepad.a_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_A) == XINPUT_GAMEPAD_A);
    EXPullDigitalButton(&state->gamepad.b_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_B) == XINPUT_GAMEPAD_B);
    EXPullDigitalButton(&state->gamepad.x_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_X) == XINPUT_GAMEPAD_X);
    EXPullDigitalButton(&state->gamepad.y_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) == XINPUT_GAMEPAD_Y);
    EXPullAnalogButton(&state->gamepad.left_trigger, xinput_state.Gamepad.bLeftTrigger / 255.0f);
    EXPullAnalogButton(&state->gamepad.right_trigger, xinput_state.Gamepad.bRightTrigger / 255.0f);
#define CONVERT(x) (2.0f * (((x + 32768) / 65535.0f) - 0.5f))
    EXPullStick(&state->gamepad.left_thumb_stick, CONVERT(xinput_state.Gamepad.sThumbLX), CONVERT(xinput_state.Gamepad.sThumbLY));
    EXPullStick(&state->gamepad.right_thumb_stick, CONVERT(xinput_state.Gamepad.sThumbRX), CONVERT(xinput_state.Gamepad.sThumbRY));
#undef CONVERT
}

void
EXPushGamepad(EXMU *state) {
    uint16_t left_motor_speed = state->gamepad.left_motor_speed * UINT16_MAX;
    uint16_t right_motor_speed = state->gamepad.right_motor_speed * UINT16_MAX;
    
    XINPUT_VIBRATION xinput_vibration;
    xinput_vibration.wLeftMotorSpeed = left_motor_speed;
    xinput_vibration.wRightMotorSpeed = right_motor_speed;
    state->win32.xinput_set_state(0, &xinput_vibration);
}

void
EXExitWithError(EXMU *state) {
    MessageBoxA(0, state->error, "EXMU ERROR", MB_ICONERROR | MB_OK);
    ExitProcess(0);
}

void
EXWarn(EXMU *state) {
    MessageBoxA(0, state->warn, "EXMU WARN", MB_ICONWARNING | MB_OK);
}

EXBOOL
EXPull(EXMU *state) {
    if (!state->initialized) {
        if (!state->error) state->error = "EXMU was not initialized.";
        EXExitWithError(state);
        return EX_FALSE;
    }    
    EXPullWindow(state);
    EXPullTime(state);
    EXPullKeys(state);
    EXPullMouse(state);
    EXPullGamepad(state);
    return !state->quit;
}

EXBOOL
EXPush(EXMU *state) {
    if (!state->initialized) {
        if (!state->error) state->error = "EXMU was not initialized.";
        EXExitWithError(state);
        return EX_FALSE;
    }
    EXPushGamepad(state);
    return !state->quit;
}

void
EXUpdate(EXMU *state) {
    EXPush(state);
    EXPull(state);
}

LRESULT CALLBACK
EXWindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;
    EXMU *state = (EXMU*)GetWindowLongPtrA(window, GWLP_USERDATA);
    switch (message) {
    case WM_INPUT: {
        UINT size;
        GetRawInputData((HRAWINPUT)lparam, RID_INPUT, 0, &size, sizeof(RAWINPUTHEADER));
        char *buffer = (char *)_alloca(size);
        if (GetRawInputData((HRAWINPUT)lparam, RID_INPUT, buffer, &size, sizeof(RAWINPUTHEADER)) == size) {
            RAWINPUT *raw_input = (RAWINPUT *)buffer;
            if (raw_input->header.dwType == RIM_TYPEMOUSE && raw_input->data.mouse.usFlags == MOUSE_MOVE_RELATIVE){
                state->mouse.delta_position.x += raw_input->data.mouse.lLastX;
                state->mouse.delta_position.y += raw_input->data.mouse.lLastY;

                USHORT button_flags = raw_input->data.mouse.usButtonFlags;
                
                EXBOOL left_button_down = state->mouse.left_button.down;
                if (button_flags & RI_MOUSE_LEFT_BUTTON_DOWN) left_button_down = EX_TRUE;
                if (button_flags & RI_MOUSE_LEFT_BUTTON_UP) left_button_down = EX_FALSE;
                EXPullDigitalButton(&state->mouse.left_button, left_button_down);

                EXBOOL right_button_down = state->mouse.right_button.down;
                if (button_flags & RI_MOUSE_RIGHT_BUTTON_DOWN) right_button_down = EX_TRUE;
                if (button_flags & RI_MOUSE_RIGHT_BUTTON_UP) right_button_down = EX_FALSE;
                EXPullDigitalButton(&state->mouse.right_button, right_button_down);

                if (raw_input->data.mouse.usButtonFlags & RI_MOUSE_WHEEL) {
                    state->mouse.delta_wheel += ((SHORT)raw_input->data.mouse.usButtonData) / WHEEL_DELTA;
                }
            }
        }
        result = DefWindowProcA(window, message, wparam, lparam);
    } break;
    case WM_CHAR: {
        WCHAR utf16_character = (WCHAR)wparam;
        char ascii_character;
        uint32_t ascii_length = WideCharToMultiByte(CP_ACP, 0, &utf16_character, 1, &ascii_character, 1, 0, 0);
        if (ascii_length == 1 && state->text_end + 1 < state->text_buffer + sizeof(state->text_buffer) - 1) {
            *state->text_end = ascii_character;
            state->text_end[1] = 0;
            state->text_end += ascii_length;
        }
    } break;
    case WM_SIZING: {
        state->window.resized = EX_TRUE;
    } break;
    case WM_DESTROY: {
        state->quit = EX_TRUE;
    } break;
    case WM_ENTERMENULOOP:
    case WM_ENTERSIZEMOVE: {
        SetTimer(window, 0, 1, 0);
    } break;
    case WM_EXITMENULOOP:
    case WM_EXITSIZEMOVE: {
        KillTimer(window, 0);
    } break;
    case WM_TIMER: {
        SwitchToFiber(state->win32.main_fiber);
    } break;
    default:
        result = DefWindowProcA(window, message, wparam, lparam);
    }
    return result;
}

void CALLBACK
EXMessageFiberProc(EXMU *state) {
    //SetTimer(state->win32.window, 0, 1, 0);
    for (;;) {
        MSG message;
        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
        SwitchToFiber(state->win32.main_fiber);
    }
}

EXBOOL
EXWindowInitialize(EXMU *state) {
    if (!state->window.title) state->window.title = "EXMU";

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
    window_class.lpszClassName = "EXMU";
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    if (RegisterClassA(&window_class) == 0) {
        state->error = "Failed to register window class.";
        return EX_FALSE;
    }

    state->win32.window = CreateWindow("EXMU", state->window.title, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                       window_x, window_y, window_width, window_height,
                                       0, 0, 0, 0);
    if (!state->win32.window) {
        state->error = "Failed to create window.";
        return EX_FALSE;
    }

    SetWindowLongPtr(state->win32.window, GWLP_USERDATA, (LONG_PTR)state);
    return EX_TRUE;
}

EXBOOL
EXTimeInitialize(EXMU *state) {
    LARGE_INTEGER large_integer;
    QueryPerformanceFrequency(&large_integer);
    state->time.ticks_per_second = large_integer.QuadPart;
    QueryPerformanceCounter(&large_integer);
    state->time.initial_ticks = large_integer.QuadPart;
    return EX_TRUE;
}

EXBOOL
EXMouseInitialize(EXMU *state) {
    RAWINPUTDEVICE raw_input_device = {};
    raw_input_device.usUsagePage = 0x01;
    raw_input_device.usUsage = 0x02;
    raw_input_device.hwndTarget = state->win32.window;
    if (!RegisterRawInputDevices(&raw_input_device, 1, sizeof(raw_input_device))) {
        state->error = "Failed to register mouse as raw input device.";
        return EX_FALSE;
    }
    return EX_TRUE;
}

EXBOOL
EXGamepadInitialize(EXMU *state) {    
    state->win32.xinput_get_state = xinput_get_state_;
    state->win32.xinput_set_state = xinput_set_state_;
    
    HMODULE xinput_module = LoadLibraryA("xinput1_3.dll");
    if (xinput_module) {
        state->win32.xinput_get_state = (XINPUTGETSTATE *)GetProcAddress(xinput_module, "XInputGetState");
        state->win32.xinput_set_state = (XINPUTSETSTATE *)GetProcAddress(xinput_module, "XInputSetState");
    } else {
        state->warn = "Failed to load xinput1_3.dll";
        EXWarn(state);
    }
    
    float trigger_threshold = XINPUT_GAMEPAD_TRIGGER_THRESHOLD / 255.0f;
    state->gamepad.left_trigger.threshold = trigger_threshold;
    state->gamepad.right_trigger.threshold = trigger_threshold;
    state->gamepad.left_thumb_stick.threshold = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / 32767.0f;;
    state->gamepad.right_thumb_stick.threshold = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / 32767.0f;
    return EX_TRUE;
}

EXBOOL
EXInitialize(EXMU *state) {
    if (!EXWindowInitialize(state)) return EX_FALSE;
    if (!EXTimeInitialize(state)) return EX_FALSE;
    if (!EXMouseInitialize(state)) return EX_FALSE;
    if (!EXGamepadInitialize(state)) return EX_FALSE;
    
    state->initialized = EX_TRUE;
    EXPull(state);
    return EX_TRUE;
}

void
EXPrint(const char *format, ...) {
    static char buffer[1024];
    va_list arg_list;
    va_start(arg_list, format);
    vsprintf_s(buffer, sizeof(buffer), format, arg_list);
    va_end(arg_list);
    printf(buffer);
}

EXMU exmu;

int main() {
    exmu.window.size.x = 1920 * 0.5;
    exmu.window.size.y = 1080 * 0.5;
    EXInitialize(&exmu);
    while (!exmu.quit) {
#if 0
        static double last_print_time = 0.0f;
        if (exmu.time.seconds - last_print_time > 0.1f) {
            EXPrint("x: %d y: %d, dx: %d dy: %d\n", exmu.window.position.x, exmu.window.position.y, exmu.window.size.x, exmu.window.size.y);
            EXPrint("delta_ticks: %llu, time_ticks: %llu\n", exmu.time.delta_ticks, exmu.time.ticks);
            EXPrint("ns: %llu, us: %llu, ms: %llu\n", exmu.time.nanoseconds, exmu.time.microseconds, exmu.time.milliseconds);
            EXPrint("ds: %f, dns: %llu, dus: %llu, dms: %llu\n", exmu.time.delta_seconds, exmu.time.delta_nanoseconds, exmu.time.delta_microseconds, exmu.time.delta_milliseconds);
            last_print_time = exmu.time.seconds;
        }
#endif
        
#if 0
        if (exmu.text)
            EXPrint("New text: %s\n", exmu.text);
        if (exmu.keyboard.keys[EX_KEY_ESCAPE].pressed)
            exmu.quit = EX_TRUE;
        if (exmu.keyboard.keys[EX_KEY_CONTROL].pressed)
            EXPrint("Control key pressed\n");
        if (exmu.keyboard.keys[EX_KEY_CONTROL].released)
            EXPrint("Control key released\n");
#endif
        
#if 0
        if (exmu.mouse.left_button.pressed) EXPrint("LMB pressed: %d, %d\n", exmu.mouse.position.x, exmu.mouse.position.y);
        if (exmu.mouse.left_button.released) EXPrint("LMB released.\n");
        if (exmu.mouse.right_button.pressed) EXPrint("RMB pressed.\n");
        if (exmu.mouse.right_button.released) EXPrint("RMB released.\n");
        if (exmu.mouse.delta_wheel) EXPrint("Wheel delta: %d\nWheel: %d\n", exmu.mouse.delta_wheel, exmu.mouse.wheel);
#endif
        
#if 0
        if (exmu.gamepad.left_shoulder_button.pressed) EXPrint("Left shoulder button pressed.\n");
        if (exmu.gamepad.left_shoulder_button.released) EXPrint("Left shoulder button released.\n");
        if (exmu.gamepad.right_shoulder_button.pressed) EXPrint("Right shoulder button pressed.\n");
        if (exmu.gamepad.right_shoulder_button.released) EXPrint("Right shoulder button released.\n");        
        if (exmu.gamepad.a_button.pressed) EXPrint("A button pressed.\n");
        if (exmu.gamepad.a_button.released) EXPrint("A button released.\n");
        if (exmu.gamepad.b_button.pressed) EXPrint("B button pressed.\n");
        if (exmu.gamepad.b_button.released) EXPrint("B button released.\n");
        if (exmu.gamepad.x_button.pressed) EXPrint("X button pressed.\n");
        if (exmu.gamepad.x_button.released) EXPrint("X button released.\n");
        if (exmu.gamepad.y_button.pressed) EXPrint("Y button pressed.\n");
        if (exmu.gamepad.y_button.released) EXPrint("Y button released.\n");
        if (exmu.gamepad.left_trigger.pressed) EXPrint("Left trigger pressed.\n");
        if (exmu.gamepad.left_trigger.released) EXPrint("Left trigger released.\n");
        if (exmu.gamepad.right_trigger.pressed) EXPrint("Right trigger pressed.\n");
        if (exmu.gamepad.right_trigger.released) EXPrint("Right trigger released.\n");
        if (exmu.gamepad.left_thumb_stick.x) EXPrint("Left thumbstick x: %f\n", exmu.gamepad.left_thumb_stick.x);
        if (exmu.gamepad.left_thumb_stick.y) EXPrint("Left thumbstick y: %f\n", exmu.gamepad.left_thumb_stick.y);

        //exmu.gamepad.left_motor_speed = 0.5f;
        //exmu.gamepad.right_motor_speed = 0.5f;
#endif
        //if (exmu.window.resized) EXPrint("Window resized.\n");
        EXUpdate(&exmu);
    }
    return 0;
}
