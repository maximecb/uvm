#include <uvm/syscalls.h>
#include <uvm/utils.h>
#include <uvm/graphics.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define FRAME_WIDTH 512
#define FRAME_HEIGHT 512

// RGBA pixels
u32 frame_buffer[512][512];

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
    u64 start_time = time_current_ms();
    float time_f = (float)(int)start_time / 1000.0f;

    // Clear the frame buffer, set all pixels to black
    memset32(frame_buffer, 0, sizeof(frame_buffer) / 4);



    /*
    for (int y = 0; y < FRAME_HEIGHT; ++y)
    for (int x = 0; x < FRAME_WIDTH; ++x)
    {
        int color = (int)(128.0f + (128.0f * sinf((float)x / 8.0f)));
        frame_buffer[y][x] = rgb32(color, color, color);
    }
    */



    for (int y = 0; y < FRAME_HEIGHT; ++y)
    for (int x = 0; x < FRAME_WIDTH; ++x)
    {
        float xf = (float)x / (float)FRAME_WIDTH;
        float yf = (float)y / (float)FRAME_HEIGHT;

        u32 c = hsv_to_rgb(360.0f * xf, yf, 1.0f);

        frame_buffer[y][x] = c;
    }





    window_draw_frame(0, frame_buffer);

    // Schedule a fixed rate update for the next frame (30fps)
    fixed_rate_update(start_time, 1000 / 30, anim_callback);
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
    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Demoscene Plasma Effect", 0);
    window_on_keydown(0, keydown);

    time_delay_cb(0, anim_callback);

    enable_event_loop();
}
