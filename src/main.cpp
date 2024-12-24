#include "exmu.h"
#include <math.h>
#include <gl/gl.h>
#include <gl/glu.h>

#define P2 M_PI / 2
#define P3 3 * M_PI / 2

#define ORIGINAL_TILE_SIZE 16
#define SCREEN_ROWS 15
#define SCREEN_COLUMNS 20

#define SCALE 3
#define TILE_SIZE (ORIGINAL_TILE_SIZE * SCALE)
#if 1
#define WINDOW_WIDTH (TILE_SIZE * SCREEN_COLUMNS)
#define WINDOW_HEIGHT (TILE_SIZE * SCREEN_ROWS)
#endif

EXMU exmu;

struct PLAYER {
    EXFLOAT2 position;
    EXFLOAT2 delta_position;
    float angle;
    float move_speed;
    float rotation_speed;
} player;

struct WORLD {
    int dimension;
    int tile_size;
} world;

float length(EXFLOAT2 a, EXFLOAT2 b) {
    return (sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y)));
}

double to_radians(double degrees) {
    return degrees * (M_PI / 180.0);
}

int main() {
    exmu.window.size.x = WINDOW_WIDTH;
    exmu.window.size.y = WINDOW_HEIGHT;
    exmu.window.centered = EX_TRUE;
    EXMU_initialize(&exmu);

    world.dimension = 8;
    world.tile_size = TILE_SIZE;

    int map[world.dimension * world.dimension] = {
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 1, 0, 0, 0, 0, 1,
        1, 0, 1, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 0, 1, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 1,
        1, 1, 1, 1, 1, 1, 1, 1,
    };

    player.position.x = TILE_SIZE * (world.dimension / 2);
    player.position.y = TILE_SIZE * (world.dimension / 2);
    player.angle = 0.0f;
    player.move_speed = 2.0f;
    player.rotation_speed = 0.05f;
    player.delta_position.x = cos(player.angle) * player.move_speed;
    player.delta_position.y = sin(player.angle) * player.move_speed;
    
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    gluOrtho2D(0, exmu.window.size.x, exmu.window.size.y, 0);

    while (!exmu.quit) {
        EXMU_pull(&exmu);
        
        if (exmu.keyboard.keys[EX_KEY_ESCAPE].pressed) exmu.quit = EX_TRUE;
        if (exmu.gamepad.start_button.pressed) exmu.quit = EX_TRUE;
        
        if (exmu.gamepad.left_thumb_stick.y > 0) {
            player.position.x += player.delta_position.x;
            player.position.y += player.delta_position.y;
        }
        if (exmu.gamepad.left_thumb_stick.y < 0) {
            player.position.x -= player.delta_position.x;
            player.position.y -= player.delta_position.y;
        }
        if (exmu.gamepad.right_thumb_stick.x < 0) {
            player.angle -= player.rotation_speed;
            if (player.angle < 0.0f) player.angle += 2 * M_PI;
            player.delta_position.x = cos(player.angle) * player.move_speed;
            player.delta_position.y = sin(player.angle) * player.move_speed;
        }
        if (exmu.gamepad.right_thumb_stick.x > 0) {
            player.angle += player.rotation_speed;
            if (player.angle > 2 * M_PI) player.angle -= 2 * M_PI;
            player.delta_position.x = cos(player.angle) * player.move_speed;
            player.delta_position.y = sin(player.angle) * player.move_speed;
        }
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int ray_count = 80;
        double fov = 80.0;
        EXINT2 map_index_pos;
        int map_index;
        int depth_of_field;
        float distance;
        EXFLOAT2 ray_position;
        EXFLOAT2 ray_offset;
        float ray_angle = player.angle - (float)to_radians(ray_count / 2);
        if (ray_angle < 0) ray_angle += 2 * M_PI;
        if (ray_angle > 2 * M_PI) ray_angle -= 2 * M_PI;
        for (int ray = 0; ray < ray_count; ray++) {
            // HORIZONTAL CHECK
            depth_of_field = 0;
            float horizontal_distance = 1000000.0f;
            EXFLOAT2 horizontal_pos = player.position;
            float atan = -1 / tan(ray_angle);
            if (ray_angle > M_PI) {
                ray_position.y = (((int)player.position.y / TILE_SIZE) * TILE_SIZE) -0.0001;
                ray_position.x = (player.position.y - ray_position.y) * atan + player.position.x;
                ray_offset.y = -TILE_SIZE;
                ray_offset.x = -ray_offset.y * atan;
            }
            if (ray_angle < M_PI) {
                ray_position.y = (((int)player.position.y / TILE_SIZE) * TILE_SIZE) + TILE_SIZE;
                ray_position.x = (player.position.y - ray_position.y) * atan + player.position.x;
                ray_offset.y = TILE_SIZE;
                ray_offset.x = -ray_offset.y * atan;
            }
            if (ray_angle == 0 || ray_angle == M_PI) {
                ray_position.x = player.position.x;
                ray_position.y = player.position.y;
                depth_of_field = 8;
            }
            while (depth_of_field < 8) {
                map_index_pos.x = (int)(ray_position.x) / TILE_SIZE;
                map_index_pos.y = (int)(ray_position.y) / TILE_SIZE;
                map_index = + map_index_pos.x + map_index_pos.y * world.dimension;
                if (map_index > 0 && map_index < world.dimension * world.dimension && map[map_index] == 1) {
                    horizontal_pos = ray_position;
                    horizontal_distance = length(player.position, horizontal_pos);
                    depth_of_field = 8;
                } else {
                    ray_position.x += ray_offset.x;
                    ray_position.y += ray_offset.y;
                    depth_of_field += 1;
                }
            }
            
            // VERTICAL CHECK
            depth_of_field = 0;
            float vertical_distance = 1000000.0f;
            EXFLOAT2 vertical_pos = player.position;
            float ntan = -tan(ray_angle);
            if (ray_angle > P2 && ray_angle < P3) {
                ray_position.x = (((int)player.position.x / TILE_SIZE) * TILE_SIZE) -0.0001;
                ray_position.y = (player.position.x - ray_position.x) * ntan + player.position.y;
                ray_offset.x = -TILE_SIZE;
                ray_offset.y = -ray_offset.x * ntan;
            }
            if (ray_angle < P2 || ray_angle > P3) {
                ray_position.x = (((int)player.position.x / TILE_SIZE) * TILE_SIZE) + TILE_SIZE;
                ray_position.y = (player.position.x - ray_position.x) * ntan + player.position.y;
                ray_offset.x = TILE_SIZE;
                ray_offset.y = -ray_offset.x * ntan;
            }
            if (ray_angle == 0 || ray_angle == M_PI) {
                ray_position.x = player.position.x;
                ray_position.y = player.position.y;
                depth_of_field = 8;
            }
            while (depth_of_field < 8) {
                map_index_pos.x = (int)(ray_position.x) / TILE_SIZE;
                map_index_pos.y = (int)(ray_position.y) / TILE_SIZE;
                map_index = + map_index_pos.x + map_index_pos.y * world.dimension;
                if (map_index > 0 && map_index < world.dimension * world.dimension && map[map_index] == 1) {
                    vertical_pos = ray_position;
                    vertical_distance = length(player.position, vertical_pos);
                    depth_of_field = 8;
                } else {
                    ray_position.x += ray_offset.x;
                    ray_position.y += ray_offset.y;
                    depth_of_field += 1;
                }
            }

            if (vertical_distance < horizontal_distance) {
                ray_position = vertical_pos;
                distance = vertical_distance;
                glColor3f(0.4f, 0.4f, 0.4f);
            }
            if (horizontal_distance < vertical_distance) {
                ray_position = horizontal_pos;
                distance = horizontal_distance;
                glColor3f(0.2f, 0.2f, 0.2f);
            }
            
            // DRAW 3D WALLS
            float ca = player.angle - ray_angle;
            if (ca < 0.0f) ca += 2 * M_PI;
            if (ca > 2 * M_PI) ca -= 2 * M_PI;
            distance = distance * cos(ca);
            float line_height = (TILE_SIZE * WINDOW_HEIGHT) / distance;
            if (line_height > WINDOW_HEIGHT) line_height = WINDOW_HEIGHT;
            float line_offset = (WINDOW_HEIGHT - line_height) / 2;
            glLineWidth(10.0f);
            glBegin(GL_LINES);
            glVertex2i(6.0f + ray * (WINDOW_WIDTH / fov), line_offset);
            glVertex2i(6.0f + ray * (WINDOW_WIDTH / fov), line_height + line_offset);
            glEnd();

            ray_angle += (float)to_radians(1.0);
            if (ray_angle < 0) ray_angle += 2 * M_PI;
            if (ray_angle > 2 * M_PI) ray_angle -= 2 * M_PI;
        }
        
        EXMU_push(&exmu);
    }
    return 0;
}
