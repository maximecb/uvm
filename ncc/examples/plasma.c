#include <uvm/syscalls.h>
#include <uvm/utils.h>
#include <uvm/graphics.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define FRAME_WIDTH 512
#define FRAME_HEIGHT 512

u64 prog_start_time;

// RGBA pixels
u32 frame_buffer[FRAME_HEIGHT][FRAME_WIDTH];

// Palette of RGB colors
u32 palette[256];

// Greyscale plasma values
int plasma[FRAME_HEIGHT][FRAME_WIDTH];

// Convert a color from HSV format to RGB format
u32 hsv_to_rgb(float h, float s, float v)
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
    int time_ms_i = (int)(frame_start_time - prog_start_time);
    int palette_offs = time_ms_i / 20;

    // Clear the frame buffer, set all pixels to black
    memset32(frame_buffer, 0, sizeof(frame_buffer) / 4);

    // Draw the plasma with a shifted palette
    for (int y = 0; y < FRAME_HEIGHT; ++y)
    {
        for (int x = 0; x < FRAME_WIDTH; ++x)
        {
            frame_buffer[y][x] = palette[(plasma[y][x] + palette_offs) % 256];
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
    prog_start_time = time_current_ms();

    // Generate the palette
    for (int i = 0; i < 256; ++i)
    {
        // Vary the hue through the palette
        palette[i] = hsv_to_rgb(360.0f / 256.0f * (float)i, 1.0f, 1.0f);
    }

    // Generate the greyscale plasma values
    // Based on a tutorial by Lode Vandevenne
    // https://lodev.org/cgtutor/plasma.html
    for (int y = 0; y < FRAME_HEIGHT; ++y)
    {
        for (int x = 0; x < FRAME_WIDTH; ++x)
        {
            float dx1 = (float)x - 128;
            float dy1 = (float)y - 128;
            float d1 = sqrtf(dx1*dx1 + dy1*dy1) / 7.0f;

            float dx2 = (float)x - 300;
            float dy2 = (float)y - 306;
            float d2 = sqrtf(dx2*dx2 + dy2*dy2) / 5.0f;

            // Sum of multiple sine functions, divided by number of sines
            plasma[y][x] = (int)(
                  128.0f + (128.0f * sinf((float)x / 12.0f))
                + 128.0f + (128.0f * sinf((float)y / 35.0f))
                + 128.0f + (128.0f * sinf(d1))
                + 128.0f + (128.0f * sinf(d2))
            ) / 4;
        }
    }

    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Demoscene Plasma Effect", 0);
    window_on_keydown(0, keydown);

    time_delay_cb(0, anim_callback);

    enable_event_loop();
}
