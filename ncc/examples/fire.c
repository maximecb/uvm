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
    if (t < 1.0f/6)
        return p + (q - p) * 6 * t;
    if (t < 1.0f/2)
        return q;
    if (t < 2.0f/3)
        return p + (q - p) * (2.0f/3 - t) * 6;

    return p;
}

//
// Converts an HSL color value to RGB. Conversion formula
// adapted from http://en.wikipedia.org/wiki/HSL_color_space.
// Assumes h, s, and l are contained in [0, 1] and
// returns RGB in [0, 255].
//
/*
u32 hsl_to_rgb(float h, float s, float l)
{
    if (s == 0)
    {
        // Achromatic
        result.r = result.g = result.b = l;
    }
    else
    {
        float q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        float p = 2 * l - q;
        result.r = hue2rgb(p, q, h + 1./3) * 255;
        result.g = hue2rgb(p, q, h) * 255;
        result.b = hue2rgb(p, q, h - 1./3) * 255;
    }

    return result;
}
*/





// TODO
// Convert a color from HSL format to RGB format
u32 hsl_to_rgb(float h, float s, float v)
{
	if (s < 0.01f)
	{
		// simple gray conversion
        int c = (int)(v * 255.0f);
        return rgb32(c, c, c);
	}

    // convert hue from [0, 360( to range [0,6)
    h = h / 60.0f;
    if (h >= 6.0f)
        h = h - 6.0f;

    // break "h" down into integer and fractional parts.
    int i = (int)h;
    float f = h - (float)i;

    // Compute the permuted RGB values
    int vi = (int)(f * 255.0f);
    int p = (int)((v * (1.0f - s)) * 255.0f);
    int q = (int)((v * (1.0f - (s * f))) * 255.0f);
    int t = (int)((v * (1.0f - (s * (1.0f - f)))) * 255.0f);

    // map v, p, q, and t into red, green, and blue values
    if (i == 0)
        return rgb32(vi, t, p);
    if (i == 1)
        return rgb32(q, vi, p);
    if (i == 2)
        return rgb32(p, vi, t);
    if (i == 3)
        return rgb32(p, q, vi);
    if (i == 4)
        return rgb32(t, p, vi);

    return rgb32(vi, p, q);
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
        palette[i] = hsl_to_rgb(360.0f / 256.0f * (float)i, 1.0f, 1.0f);
    }

    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Demoscene Fire Effect", 0);
    window_on_keydown(0, keydown);

    time_delay_cb(0, anim_callback);

    enable_event_loop();
}
