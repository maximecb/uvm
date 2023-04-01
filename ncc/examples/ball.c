#include <uvm/syscalls.h>
#include <uvm/utils.h>
#include <stdlib.h>
#include <stdint.h>

#define FRAME_WIDTH 800
#define FRAME_HEIGHT 600
#define BALL_RADIUS 20
#define AUDIO_LEN 8_000

// RGBA pixels: 800 * 600
u32 frame_buffer[480_000];

// Current ball position
int px = 200;
int py = 200;

// Velocity
int vx = 5;
int vy = 7;

// Buffer used for audio output
u16 AUDIO_BUFFER[1024];

// Current position in the synthesized sound effect
u32 audio_pos = UINT32_MAX;

// Draw the ball at the current x,y position
void draw_ball()
{
    size_t xmin = px;
    size_t xmax = px + BALL_RADIUS;
    if (px >= BALL_RADIUS) xmin = px - BALL_RADIUS;
    if (xmax >= FRAME_WIDTH) xmax = FRAME_WIDTH;

    size_t ymin = py;
    size_t ymax = py + BALL_RADIUS;
    if (py >= BALL_RADIUS) ymin = py - BALL_RADIUS;
    if (ymax >= FRAME_HEIGHT) ymax = FRAME_HEIGHT;

    for (size_t x = xmin; x < xmax; ++x)
    {
        for (size_t y = ymin; y < ymax; ++y)
        {
            size_t dx = x - px;
            size_t dy = y - py;
            size_t dist_sqr = dx * dx + dy * dy;

            if (dist_sqr <= BALL_RADIUS * BALL_RADIUS)
            {
                u32* pix_ptr = frame_buffer + (FRAME_WIDTH * y + x);
                *pix_ptr = 0xFF_00_00;
            }
        }
    }
}

void anim_callback()
{
    u64 start_time = time_current_ms();

    // Clear the frame buffer, set all pixels to black
    memset32(frame_buffer, 0, 800 * 600);

    draw_ball();

    px = px + vx;
    py = py + vy;

    if (px + BALL_RADIUS > FRAME_WIDTH)
    {
        vx = -vx;
        audio_pos = 0;
    }
    if (px - BALL_RADIUS < 0)
    {
        vx = -vx;
        audio_pos = 0;
    }

    if (py + BALL_RADIUS > FRAME_HEIGHT)
    {
        vy = -vy;
        audio_pos = 0;
    }
    if (py - BALL_RADIUS < 0)
    {
        vy = -vy;
        audio_pos = 0;
    }

    window_draw_frame(0, frame_buffer);

    // Schedule a fixed rate update for the next frame (60fps)
    fixed_rate_update(start_time, 1000 / 60, anim_callback);
}

u16* audio_cb(u16 num_channels, u32 num_samples)
{
    assert(num_channels == 1);
    assert(num_samples <= 1024);

    memset(AUDIO_BUFFER, 0, sizeof(AUDIO_BUFFER));

    if (audio_pos > AUDIO_LEN)
    {
        return AUDIO_BUFFER;
    }

    // TODO: synthesize a more "boing-like" sound effect
    // Pull requests welcome! :)
    for (int i = 0; i < num_samples && audio_pos < AUDIO_LEN; ++i)
    {
        // The intensity decreases over time
        u32 intensity = INT16_MAX - (audio_pos * INT16_MAX / AUDIO_LEN);

        u32 sawtooth = 4000 * (i % 128) / 128;
        AUDIO_BUFFER[i] = intensity * sawtooth / INT16_MAX;

        ++audio_pos;
    }

    return AUDIO_BUFFER;
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
    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Bouncing Ball Example", 0);
    window_on_keydown(0, keydown);

    time_delay_cb(0, anim_callback);

    audio_open_output(44100, 1, AUDIO_FORMAT_I16, audio_cb);

    enable_event_loop();
}
