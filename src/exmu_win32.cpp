#include "exmu.h"

#define NO_STRICT
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <xinput.h>

#ifdef _DEBUG
#define EX_ASSERT(x) \
    if (!(x)) { MessageBoxA(0, #x, "Assertion Failure", MB_OK); __debugbreak(); }
#else
#define EX_ASSERT(x)
#endif

X_INPUT_GET_STATE(xinput_get_state_) { return 0; }
X_INPUT_SET_STATE(xinput_set_state_) { return 0; }

void
EXMU_exit_with_error(EXMU *state) {
    MessageBoxA(0, state->error, "EXMU ERROR", MB_ICONERROR | MB_OK);
    ExitProcess(0);
}


void
EXMU_update_digital_button(EXDIGITALBUTTON *button, EXBOOL down) {
    EXBOOL was_down = button->down;
    button->pressed = !was_down && down;
    button->released = was_down && !down;
    button->down = down;
}

void
EXMU_update_analog_button(EXANALOGBUTTON *button, float value) {
    button->value = value;
    EXBOOL was_down = button->down;
    button->down = (value >= button->threshold);
    button->pressed = !was_down && button->down;
    button->released = was_down && !button->down;
}

void
EXMU_update_stick(EXSTICK *stick, float x, float y) {    
    if (fabs(x) <= stick->threshold) x = 0.0f;
    stick->x = x;
    
    if (fabs(y) <= stick->threshold) y = 0.0f;
    stick->y = y;
}

void
EXMU_window_pull(EXMU *state) {
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
EXMU_time_pull(EXMU *state) {
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
EXMU_keyboard_pull(EXMU *state) {
    BYTE keyboard_state[EX_MAX_KEYS];
    if (!GetKeyboardState(keyboard_state)) return;
    for (int key = 0; key < EX_MAX_KEYS; key++) {
        EXMU_update_digital_button(state->keyboard.keys + key, keyboard_state[key] >> 7);
    }
}

void
EXMU_mouse_pull(EXMU *state) {
    POINT mouse_position;
    GetCursorPos(&mouse_position);
    
    mouse_position.x -= state->window.position.x;
    mouse_position.y -= state->window.position.y;

    state->mouse.position.x = mouse_position.x;
    state->mouse.position.y = mouse_position.y;
}

void
EXMU_gamepad_pull(EXMU *state) {
    if (!state->win32.xinput_get_state) return;

    XINPUT_STATE xinput_state = {};
    if (state->win32.xinput_get_state(0, &xinput_state) == ERROR_DEVICE_NOT_CONNECTED) {
        state->gamepad.connected = EX_FALSE;
        return;
    }

    state->gamepad.connected = EX_TRUE;
    
    EXMU_update_digital_button(&state->gamepad.up_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) == XINPUT_GAMEPAD_DPAD_UP);
    EXMU_update_digital_button(&state->gamepad.down_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) == XINPUT_GAMEPAD_DPAD_DOWN);
    EXMU_update_digital_button(&state->gamepad.left_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) == XINPUT_GAMEPAD_DPAD_LEFT);
    EXMU_update_digital_button(&state->gamepad.right_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) == XINPUT_GAMEPAD_DPAD_RIGHT);
    EXMU_update_digital_button(&state->gamepad.start_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_START) == XINPUT_GAMEPAD_START);
    EXMU_update_digital_button(&state->gamepad.back_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) == XINPUT_GAMEPAD_BACK);
    EXMU_update_digital_button(&state->gamepad.left_thumb_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) == XINPUT_GAMEPAD_LEFT_THUMB);
    EXMU_update_digital_button(&state->gamepad.right_thumb_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) == XINPUT_GAMEPAD_RIGHT_THUMB);
    EXMU_update_digital_button(&state->gamepad.left_shoulder_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) == XINPUT_GAMEPAD_LEFT_SHOULDER);
    EXMU_update_digital_button(&state->gamepad.right_shoulder_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) == XINPUT_GAMEPAD_RIGHT_SHOULDER);
    EXMU_update_digital_button(&state->gamepad.a_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_A) == XINPUT_GAMEPAD_A);
    EXMU_update_digital_button(&state->gamepad.b_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_B) == XINPUT_GAMEPAD_B);
    EXMU_update_digital_button(&state->gamepad.x_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_X) == XINPUT_GAMEPAD_X);
    EXMU_update_digital_button(&state->gamepad.y_button, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) == XINPUT_GAMEPAD_Y);
    EXMU_update_analog_button(&state->gamepad.left_trigger, xinput_state.Gamepad.bLeftTrigger / 255.0f);
    EXMU_update_analog_button(&state->gamepad.right_trigger, xinput_state.Gamepad.bRightTrigger / 255.0f);
#define CONVERT(x) (2.0f * (((x + 32768) / 65535.0f) - 0.5f))
    EXMU_update_stick(&state->gamepad.left_thumb_stick, CONVERT(xinput_state.Gamepad.sThumbLX), CONVERT(xinput_state.Gamepad.sThumbLY));
    EXMU_update_stick(&state->gamepad.right_thumb_stick, CONVERT(xinput_state.Gamepad.sThumbRX), CONVERT(xinput_state.Gamepad.sThumbRY));
#undef CONVERT
}

EXBOOL
EXMU_pull(EXMU *state) {
    if (!state->initialized) {
        if (!state->error) state->error = "EXMU was not initialized.";
        EXMU_exit_with_error(state);
        return EX_FALSE;
    }    
    EXMU_window_pull(state);
    EXMU_time_pull(state);
    EXMU_keyboard_pull(state);
    EXMU_mouse_pull(state);
    EXMU_gamepad_pull(state);
    return !state->quit;
}

void
EXMU_gamepad_push(EXMU *state) {
    uint16_t left_motor_speed = state->gamepad.left_motor_speed * UINT16_MAX;
    uint16_t right_motor_speed = state->gamepad.right_motor_speed * UINT16_MAX;
    
    XINPUT_VIBRATION xinput_vibration;
    xinput_vibration.wLeftMotorSpeed = left_motor_speed;
    xinput_vibration.wRightMotorSpeed = right_motor_speed;
    state->win32.xinput_set_state(0, &xinput_vibration);
}

void
EXMU_opengl_push(EXMU *state) {
    SwapBuffers(state->win32.device_context);
}

EXBOOL
EXMU_push(EXMU *state) {
    if (!state->initialized) {
        if (!state->error) state->error = "EXMU was not initialized.";
        EXMU_exit_with_error(state);
        return EX_FALSE;
    }
    EXMU_gamepad_push(state);
    EXMU_opengl_push(state);
    return !state->quit;
}

LRESULT CALLBACK
EXMU_win32_window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;
    EXMU *state = (EXMU *)GetWindowLongPtrA(window, GWLP_USERDATA);
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
                EXMU_update_digital_button(&state->mouse.left_button, left_button_down);

                EXBOOL right_button_down = state->mouse.right_button.down;
                if (button_flags & RI_MOUSE_RIGHT_BUTTON_DOWN) right_button_down = EX_TRUE;
                if (button_flags & RI_MOUSE_RIGHT_BUTTON_UP) right_button_down = EX_FALSE;
                EXMU_update_digital_button(&state->mouse.right_button, right_button_down);

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
    case WM_DESTROY: {
        state->quit = EX_TRUE;
    } break;
    case WM_SIZING: {
        state->window.resized = EX_TRUE;
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
EXMU_win32_message_fiber_proc(EXMU *state) {
    SetTimer(state->win32.window, 0, 1, 0);
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
EXMU_window_initialize(EXMU *state) {
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

        if (state->window.centered) {
            window_x = (GetSystemMetrics(SM_CXSCREEN) - window_width) / 2;
            window_y = (GetSystemMetrics(SM_CYSCREEN) - window_height) / 2;
        }
    }

    state->win32.main_fiber = ConvertThreadToFiber(0);
    EX_ASSERT(state->win32.main_fiber);
    state->win32.message_fiber = CreateFiber(0, (PFIBER_START_ROUTINE)EXMU_win32_message_fiber_proc, state);
    EX_ASSERT(state->win32.message_fiber);
    
    WNDCLASSA window_class = {};
    window_class.lpfnWndProc = EXMU_win32_window_proc;
    window_class.lpszClassName = "EXMU";
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    if (RegisterClassA(&window_class) == 0) {
        state->error = "Failed to register win32 window class.";
        return EX_FALSE;
    }

    state->win32.window = CreateWindow("EXMU", state->window.title, WS_OVERLAPPEDWINDOW, window_x, window_y, window_width, window_height, 0, 0, 0, 0);
    if (!state->win32.window) {
        state->error = "Failed to create win32 window.";
        return EX_FALSE;
    }

    SetWindowLongPtr(state->win32.window, GWLP_USERDATA, (LONG_PTR)state);
    ShowWindow(state->win32.window, SW_SHOW);
    state->win32.device_context = GetDC(state->win32.window);
    return EX_TRUE;
}

EXBOOL
EXMU_time_initialize(EXMU *state) {
    LARGE_INTEGER large_integer;
    QueryPerformanceFrequency(&large_integer);
    state->time.ticks_per_second = large_integer.QuadPart;
    QueryPerformanceCounter(&large_integer);
    state->time.initial_ticks = large_integer.QuadPart;
    return EX_TRUE;
}

EXBOOL
EXMU_mouse_initialize(EXMU *state) {
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
EXMU_gamepad_initialize(EXMU *state) {    
    state->win32.xinput_get_state = xinput_get_state_;
    state->win32.xinput_set_state = xinput_set_state_;
    
    HMODULE xinput_module = LoadLibraryA("xinput1_3.dll");
    if (xinput_module) {
        state->win32.xinput_get_state = (XINPUTGETSTATE *)GetProcAddress(xinput_module, "XInputGetState");
        state->win32.xinput_set_state = (XINPUTSETSTATE *)GetProcAddress(xinput_module, "XInputSetState");
    }
    
    float trigger_threshold = XINPUT_GAMEPAD_TRIGGER_THRESHOLD / 255.0f;
    state->gamepad.left_trigger.threshold = trigger_threshold;
    state->gamepad.right_trigger.threshold = trigger_threshold;
    state->gamepad.left_thumb_stick.threshold = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / 32767.0f;;
    state->gamepad.right_thumb_stick.threshold = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / 32767.0f;
    return EX_TRUE;
}

EXBOOL
EXMU_opengl_initialize(EXMU *state) {
    PIXELFORMATDESCRIPTOR pixel_format_descriptor;
    pixel_format_descriptor.nSize = sizeof(pixel_format_descriptor);
    pixel_format_descriptor.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
    pixel_format_descriptor.iPixelType = PFD_TYPE_RGBA;
    pixel_format_descriptor.cColorBits = 24;
    pixel_format_descriptor.cAlphaBits = 0;
    pixel_format_descriptor.cDepthBits = 24;
    pixel_format_descriptor.cStencilBits = 8;
    int pixel_format = ChoosePixelFormat(state->win32.device_context, &pixel_format_descriptor);
    if (!pixel_format) {
        return EX_FALSE;
    }    
    if (!DescribePixelFormat(state->win32.device_context, pixel_format, sizeof(pixel_format_descriptor), &pixel_format_descriptor)) {
        return EX_FALSE;
    }
    if (!SetPixelFormat(state->win32.device_context, pixel_format, &pixel_format_descriptor)) {
        return EX_FALSE;
    }
    state->win32.wgl_context = wglCreateContext(state->win32.device_context);
    if (!state->win32.wgl_context) {
        return EX_FALSE;
    }
    wglMakeCurrent(state->win32.device_context, state->win32.wgl_context);
    return EX_TRUE;
}

EXBOOL
EXMU_initialize(EXMU *state) {
    if (!EXMU_window_initialize(state)) return EX_FALSE;
    if (!EXMU_time_initialize(state)) return EX_FALSE;
    if (!EXMU_mouse_initialize(state)) return EX_FALSE;
    if (!EXMU_gamepad_initialize(state)) return EX_FALSE;
    if (!EXMU_opengl_initialize(state)) return EX_FALSE;
    
    state->initialized = EX_TRUE;
    EXMU_pull(state);
    return EX_TRUE;
}
