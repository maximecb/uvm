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
    // Note that +Y in the image frame goes downwards,
    // so we have to flip the Y coordinates
    vec3 v_tmp;
    mat44_transform(persp, v0, v_tmp);
    int x0 = (int)remap(v_tmp[0], -1.0f, 1.0f, (float)FRAME_WIDTH, 0.0f);
    int y0 = (int)remap(v_tmp[1], -1.0f, 1.0f, (float)FRAME_HEIGHT, 0.0f);

    mat44_transform(persp, v1, v_tmp);
    int x1 = (int)remap(v_tmp[0], -1.0f, 1.0f, (float)FRAME_WIDTH, 0.0f);
    int y1 = (int)remap(v_tmp[1], -1.0f, 1.0f, (float)FRAME_HEIGHT, 0.0f);

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

//
// Converts a HUE to r, g or b.
// returns float in the set [0, 1].
//
float hue2rgb(float p, float q, float t)
{
    if (t < 0)
        t = t + 1;
    if (t > 1)
        t = t - 1;
    if (t < 1.0f / 6)
        return p + (q - p) * 6 * t;
    if (t < 1.0f / 2)
        return q;
    if (t < 2.0f / 3)
        return p + (q - p) * (2.0f / 3 - t) * 6;

    return p;
}

// Converts an HSL color value to RGB. Conversion formula
// adapted from http://en.wikipedia.org/wiki/HSL_color_space.
// Assumes h, s, and l are contained in [0, 1] and
// returns RGB in [0, 255].
//
u32 hsl_to_rgb(float h, float s, float l)
{
    if (s == 0)
    {
        // Achromatic
        int c = (int)(255 * l);
        return rgb32(c, c, c);
    }
    else
    {
        float q = l < 0.5f ? l * (1 + s) : l + s - l * s;
        float p = 2 * l - q;

        return rgb32(
            (int)(hue2rgb(p, q, h + 1.0f/3) * 255),
            (int)(hue2rgb(p, q, h) * 255),
            (int)(hue2rgb(p, q, h - 1.0f/3) * 255)
        );
    }
}

float line_pos = 1.0f;
float anim_time = 0.0f;

void anim_callback()
{
    u64 start_time = time_current_ms();

    // Clear the frame buffer, set all pixels to black
    memset32(frame_buffer, 0, 800 * 600);

    line_pos = line_pos - 0.02f;
    if (line_pos < 0)
        line_pos = line_pos + 1;

    printf("line_pos=%f\n", line_pos);

    // Animate the sky gradient over time
    float h = 0.70f + 0.05f * sinf(anim_time * 0.30f);
    float s = 0.95f + 0.05f * sinf(anim_time * 0.20f);
    float l_max = 0.92f + 0.08f * sinf(anim_time * 0.22f);

    // Draw sky/horizon
    for (int i = 0; i < 335; ++i)
    {
        u32 color = hsl_to_rgb(h, s, (i / 335.0f) * l_max);
        memset32((u32*)frame_buffer[i], color, FRAME_WIDTH);
    }

    vec3 v0;
    vec3 v1;

    // Horizontal lines
    for (int i = 0; i < 25; ++i)
    {
        v0[0] = -20;
        v0[1] = -1;
        v0[2] = -(0.1f + i + line_pos);

        v1[0] = 20;
        v1[1] = -1;
        v1[2] = -(0.1f + i + line_pos);

        draw_line3d(v0, v1, COLOR_PURPLE);
    }

    // Vertical lines
    for (int i = 0; i < 20; ++i)
    {
        v0[0] = -10.0f + i;
        v0[1] = -1;
        v0[2] = -20.0f;

        v1[0] = -10.0f + i;
        v1[1] = -1;
        v1[2] = -0.1f;

        draw_line3d(v0, v1, COLOR_PURPLE);
    }

    window_draw_frame(0, frame_buffer);

    u64 end_time = time_current_ms();
    printf("render time: %dms\n", end_time - start_time);

    // Schedule a fixed rate update for the next frame (60fps)
    fixed_rate_update(start_time, 1000 / 60, anim_callback);
    anim_time = anim_time + (1 / 60.0f);
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
