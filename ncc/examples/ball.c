#include <uvm/syscalls.h>
#include <uvm/utils.h>
#include <uvm/window.h>
#include <stdlib.h>
#include <stdint.h>

#define FRAME_WIDTH 800
#define FRAME_HEIGHT 600
#define BALL_RADIUS 20
#define AUDIO_LEN 8_000

// RGBA pixels
u32 frame_buffer[FRAME_HEIGHT][FRAME_WIDTH];

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
    uint32_t xmin = px;
    uint32_t xmax = px + BALL_RADIUS;
    if (px >= BALL_RADIUS) xmin = px - BALL_RADIUS;
    if (xmax >= FRAME_WIDTH) xmax = FRAME_WIDTH;

    uint32_t ymin = py;
    uint32_t ymax = py + BALL_RADIUS;
    if (py >= BALL_RADIUS) ymin = py - BALL_RADIUS;
    if (ymax >= FRAME_HEIGHT) ymax = FRAME_HEIGHT;

    for (uint32_t x = xmin; x < xmax; ++x)
    {
        for (uint32_t y = ymin; y < ymax; ++y)
        {
            uint32_t dx = x - px;
            uint32_t dy = y - py;
            uint32_t dist_sqr = dx * dx + dy * dy;

            if (dist_sqr <= BALL_RADIUS * BALL_RADIUS)
            {
                frame_buffer[y][x] = 0xFF_00_00;
            }
        }
    }
}

void update()
{
    // Clear the frame buffer, set all pixels to black
    memset32(frame_buffer, 0, sizeof(frame_buffer) / sizeof(u32));

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

        u32 period = 128 - (audio_pos * 64 / AUDIO_LEN);

        u32 sawtooth = 4000 * (i % period) / period;
        AUDIO_BUFFER[i] = intensity * sawtooth / INT16_MAX;

        ++audio_pos;
    }

    return AUDIO_BUFFER;
}

void main()
{
    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Bouncing Ball Example", 0);

    audio_open_output(44100, 1, AUDIO_FORMAT_I16, audio_cb);

    anim_event_loop(60, update);
}
