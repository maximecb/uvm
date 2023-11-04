#include <stdio.h>
#include <stdlib.h>
#include <uvm/syscalls.h>
#include <uvm/utils.h>
#include <uvm/math.h>
#include <uvm/3dmath.h>
#include <uvm/graphics.h>

#define FRAME_WIDTH 800
#define FRAME_HEIGHT 600

#define remap(v, a0, a1, b0, b1) (b0 + (b1 - b0) * ((v) - a0) / (a1 - a0))

// RGBA pixels
u32 frame_buffer[FRAME_HEIGHT][FRAME_WIDTH];

// Perspective projection matrix
mat44 persp;

// Draw a line between two 3D points
void draw_line3d(vec3 v0, vec3 v1, u32 color)
{
    vec3 v_tmp;
    mat44_transform(persp, v0, v_tmp);
    int x0 = (int)remap(v_tmp[0], -1.0f, 1.0f, 0.0f, (float)FRAME_WIDTH);
    int y0 = (int)remap(v_tmp[1], -1.0f, 1.0f, 0.0f, (float)FRAME_HEIGHT);

    mat44_transform(persp, v1, v_tmp);
    int x1 = (int)remap(v_tmp[0], -1.0f, 1.0f, 0.0f, (float)FRAME_WIDTH);
    int y1 = (int)remap(v_tmp[1], -1.0f, 1.0f, 0.0f, (float)FRAME_HEIGHT);

    draw_line_clipped(
        (u32*)frame_buffer,
        FRAME_WIDTH,
        FRAME_HEIGHT,
        (u32)x0,
        (u32)y0,
        (u32)x1,
        (u32)y1,
        color
    );
}




float line_pos = 1.0f;

void anim_callback()
{
    u64 start_time = time_current_ms();

    // Clear the frame buffer, set all pixels to black
    memset32(frame_buffer, 0, 800 * 600);



    line_pos = line_pos - 0.02f;
    if (line_pos < 0)
        line_pos = line_pos + 1;

    printf("line_pos=%f\n", line_pos);





    vec3 v0;
    vec3 v1;

    // Horizontal lines
    for (int i = 0; i < 25; ++i)
    {
        v0[0] = -20;
        v0[1] = 1;
        v0[2] = -(0.1f + i + line_pos);

        v1[0] = 20;
        v1[1] = 1;
        v1[2] = -(0.1f + i + line_pos);

        draw_line3d(v0, v1, COLOR_PURPLE);
    }

    // Vertical lines
    for (int i = 0; i < 20; ++i)
    {
        v0[0] = -10.0f + i;
        v0[1] = 1;
        v0[2] = -20.0f;

        v1[0] = -10.0f + i;
        v1[1] = 1;
        v1[2] = -0.1f;

        draw_line3d(v0, v1, COLOR_PURPLE);
    }







    window_draw_frame(0, frame_buffer);

    u64 end_time = time_current_ms();
    printf("render time: %dms\n", end_time - start_time);

    // Schedule a fixed rate update for the next frame (60fps)
    fixed_rate_update(start_time, 1000 / 60, anim_callback);
}

void keydown(u64 window_id, u16 keycode)
{
    if (keycode == KEY_ESCAPE)
    {
        exit(0);
    }
}

int main()
{
    window_create(FRAME_WIDTH, FRAME_HEIGHT, "The Grid", 0);
    window_on_keydown(0, keydown);
    time_delay_cb(0, anim_callback);
    enable_event_loop();

    // Set up the perspective projection matrix
    perspective(
        DEG2RAD(40.0f),
        (float)FRAME_WIDTH / (float)FRAME_HEIGHT,
        0.1f,   // near,
        100.0f, // far,
        persp
    );

    return 0;
}
