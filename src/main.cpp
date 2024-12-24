#include "exmu.h"

EXMU exmu;

int main() {
    exmu.window.size.x = 1920 * 0.5;
    exmu.window.size.y = 1080 * 0.5;
    EXMU_initialize(&exmu);
    while (!exmu.quit) {
        EXMU_pull(&exmu);
        if (exmu.keyboard.keys[EX_KEY_ESCAPE].pressed) exmu.quit = EX_TRUE;
        if (exmu.gamepad.left_thumb_stick.x) printf("Left stick x: %f\n", exmu.gamepad.left_thumb_stick.x);
        if (exmu.gamepad.left_thumb_stick.y) printf("Left stick y: %f\n", exmu.gamepad.left_thumb_stick.y);
        if (exmu.gamepad.right_thumb_stick.x) printf("Right stick x: %f\n", exmu.gamepad.right_thumb_stick.x);
        if (exmu.gamepad.right_thumb_stick.y) printf("Right stick y: %f\n", exmu.gamepad.right_thumb_stick.y);
        if (exmu.window.resized) printf("Window resized.\n");
        EXMU_push(&exmu);
    }
    return 0;
}
