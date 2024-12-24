#include "exmu.h"
#include <gl/gl.h>

EXMU exmu;

int main() {
    exmu.window.size.x = 1920 * 0.5;
    exmu.window.size.y = 1080 * 0.5;
    exmu.window.centered = EX_TRUE;
    EXMU_initialize(&exmu);
    while (!exmu.quit) {
        EXMU_pull(&exmu);
        
        if (exmu.keyboard.keys[EX_KEY_ESCAPE].pressed) exmu.quit = EX_TRUE;
        glViewport(0, 0, exmu.window.size.x, exmu.window.size.y);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.5f, 0.0f);
        glVertex2f(-0.5f, -0.5f);
        glVertex2f(0.5f, -0.5f);
        glVertex2f(0.0f, 0.5f);
        glEnd();
        
        EXMU_push(&exmu);
    }
    return 0;
}
