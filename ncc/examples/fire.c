// Demoscene-style fire effect
// Based on a tutorial by Lode Vandevenne:
// https://lodev.org/cgtutor/fire.html

#include <uvm/syscalls.h>
#include <uvm/utils.h>
#include <uvm/graphics.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define FRAME_WIDTH 512
#define FRAME_HEIGHT 512

// RGBA pixels
u32 frame_buffer[FRAME_HEIGHT][FRAME_WIDTH];

// Palette of RGB colors
u32 palette[256];

// Greyscale fire values
int fire[FRAME_HEIGHT][FRAME_WIDTH];

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

void anim_callback()
{
    u64 frame_start_time = time_current_ms();

    // Clear the frame buffer, set all pixels to black
    memset32(frame_buffer, 0, sizeof(frame_buffer) / 4);

    // Randomize the bottom row
    for (int x = 0; x < FRAME_WIDTH; ++x)
    {
        int r = abs(rand()) % 256;
        assert(r >= 0 && r < 256);
        fire[FRAME_HEIGHT-1][x] = r;
    }

    // Apply the update rule, from top to bottom
    for (int y = 0; y < FRAME_HEIGHT - 1; ++y)
    {
        for (int x = 1; x < FRAME_WIDTH - 1; ++x)
        {
            int pix0 = fire[y+1][x];
            int pix1 = fire[y+1][(x-1 + FRAME_WIDTH) % FRAME_WIDTH];
            int pix2 = fire[y+1][(x+1) % FRAME_WIDTH];
            int pix3 = fire[(y+2) % FRAME_HEIGHT][x];
            int sum = (pix0 + pix1 + pix2 + pix3) * 63 / 256;
            assert(sum < 256);
            fire[y][x] = sum;
        }
    }

    // Draw the fire
    for (int y = 0; y < FRAME_HEIGHT; ++y)
    {
        for (int x = 0; x < FRAME_WIDTH; ++x)
        {
            frame_buffer[y][x] = palette[fire[y][x]];
        }
    }

    window_draw_frame(0, frame_buffer);

    // Schedule a fixed rate update for the next frame (40fps)
    fixed_rate_update(frame_start_time, 1000 / 40, anim_callback);
}

void keydown(u64 window_id, u16 keycode)
{
    if (keycode == KEY_ESCAPE)
    {
        exit(0);
    }
}

void main()
{
    // Generate the palette
    for (int i = 0; i < 256; ++i)
    {
        // Vary the hue through the palette
        // Hue should be between orange and red
        float x = (float)i / 255;
        float l = x + 0.3f;
        l = l * l;
        if (l > 1.0f) l = 1.0f;
        palette[i] = hsl_to_rgb(x * 0.33f, 1.0f, l);
    }

    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Demoscene Fire Effect", 0);
    window_on_keydown(0, keydown);

    time_delay_cb(0, anim_callback);

    enable_event_loop();
}
