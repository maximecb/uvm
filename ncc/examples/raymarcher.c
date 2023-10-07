// Demoscene-style fire effect
// Based on a tutorial by Lode Vandevenne:
// https://lodev.org/cgtutor/fire.html

#include <uvm/syscalls.h>
#include <uvm/utils.h>
#include <uvm/graphics.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480

#define FOV_X 90
#define NEAR 0.2f

#define MAP_WIDTH 6
#define MAP_HEIGHT 6

// ----> +y
// |
// v
// +x
u8 map[MAP_HEIGHT][MAP_WIDTH] = {
    { 1, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 1 },
};

// RGBA pixels
u32 frame_buffer[FRAME_HEIGHT][FRAME_WIDTH];

// Player position and camera direction
float pos_x = 1.5f;
float pos_y = 1.5f;
float dir = 0.0f;
float dir_x = 1.0f;
float dir_y = 0.0f;

#define DEG2RAD(a) (a * M_PI_F / 180)

void march_ray(float x0, float y0, float dx, float dy)
{
    int i = (int)x0;
    int j = (int)y0;





}

void anim_callback()
{
    u64 frame_start_time = time_current_ms();

    // Clear the frame buffer, set all pixels to black
    memset32(frame_buffer, 0, sizeof(frame_buffer) / 4);

    // Compute the left vector (parallel with the image plane)
    float left_x = -dir_y;
    float left_y = dir_x;

    // Half of the image frame width
    float half_w = NEAR * tanf(FOV_X / 2);

    // Leftmost and rightmost coordinates on the image frame
    float x0 = pos_x + dir_x + (half_w * left_x);
    float y0 = pos_y + dir_y + (half_w * left_y);
    float x1 = pos_x + dir_x - (half_w * left_x);
    float y1 = pos_y + dir_y - (half_w * left_y);

    //printf("i=%d, j=%d, x=%f, y=%f, dx=%f, dy=%f\n", pos_i, pos_j, pos_x, pos_y, dir_x, dir_y);




    // For each column of the frame
    for (int i = 0; i < FRAME_WIDTH; ++i)
    {
        // Starting position of the ray
        float x0 = (x1 - x0) * i / FRAME_WIDTH;
        float y0 = (y1 - y0) * i / FRAME_WIDTH;

        // Ray direction
        float dx = x0 - pos_x;
        float dy = x1 - pos_y;
        float l = sqrtf(dx*dx + dy*dy);
        dx = dx / l;
        dy = dy / l;

        march_ray(x0, y0, dx, dy);
    }








    window_draw_frame(0, frame_buffer);

    // Schedule a fixed rate update for the next frame (30fps)
    fixed_rate_update(frame_start_time, 1000 / 30, anim_callback);
}

void keydown(u64 window_id, u16 keycode)
{
    if (keycode == KEY_ESCAPE)
    {
        exit(0);
    }

    else if (keycode == KEY_LEFT)
    {
        dir = dir + DEG2RAD(20);
        if (dir >= 2 * M_PI_F)
            dir = dir - 2 * M_PI_F;

        dir_x = cosf(dir);
        dir_y = sinf(dir);
    }

    else if (keycode == KEY_RIGHT)
    {
        dir = dir - DEG2RAD(20);
        if (dir < 0)
            dir = dir + 2 * M_PI_F;

        dir_x = cosf(dir);
        dir_y = sinf(dir);
    }

    else if (keycode == KEY_UP)
    {
        pos_x = pos_x + 0.2f * dir_x;
        pos_y = pos_y + 0.2f * dir_y;
    }

    else if (keycode == KEY_DOWN)
    {
        pos_x = pos_x - 0.2f * dir_x;
        pos_y = pos_y - 0.2f * dir_y;
    }
}

void main()
{
    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Raymarcher Example", 0);
    window_on_keydown(0, keydown);

    time_delay_cb(0, anim_callback);

    enable_event_loop();
}
