#include <uvm/syscalls.h>
#include <uvm/window.h>
#include <uvm/math.h>
#include <uvm/graphics.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define FRAME_WIDTH 600
#define FRAME_HEIGHT 200
#define SAMPLE_RATE 44100
#define DISP_SAMPLES 176400 // SAMPLE_RATE * 4

// RGBA pixels
u32 frame_buffer[FRAME_HEIGHT][FRAME_WIDTH];

// Buffer for incoming samples
i16 buffer[1024];

// Buffer for display
i16 disp_samples[DISP_SAMPLES];

// Current recording position
size_t rec_pos = 0;

void update()
{
    // Clear the frame buffer, set all pixels to black
    memset32(frame_buffer, 0, sizeof(frame_buffer) / sizeof(u32));

    int prev_y = FRAME_HEIGHT / 2;

    for (size_t x = 1; x < FRAME_WIDTH; ++x)
    {
        size_t sample_idx = x * DISP_SAMPLES / FRAME_WIDTH;

        // Bring sample into the [-1, 1] range
        float sample = (float)disp_samples[sample_idx] / (INT16_MAX + 1);

        // Bring the sample into the [0, 1] range
        sample = (sample + 1.0f) / 2;

        int y = (int)(sample * (FRAME_HEIGHT - 1));

        draw_line(
            (u32*)&frame_buffer,
            FRAME_WIDTH,
            FRAME_HEIGHT,
            x - 1,
            prev_y,
            x,
            y,
            COLOR_RED,
        );

        prev_y = y;
    }

    // Draw vertical line at recording position
    u32 rec_x = rec_pos * FRAME_WIDTH / DISP_SAMPLES;
    draw_line(
        (u32*)&frame_buffer,
        FRAME_WIDTH,
        FRAME_HEIGHT,
        rec_x,
        0,
        rec_x,
        FRAME_HEIGHT - 1,
        COLOR_WHITE,
    );

    window_draw_frame(0, frame_buffer);
}

void audio_cb(u16 num_channels, u32 num_samples)
{
    assert(num_channels == 1);
    assert(num_samples <= 1024);

    audio_read_samples(&buffer, num_samples);

    size_t end_pos = MIN(rec_pos + num_samples, sizeof(disp_samples) / sizeof(i16));
    size_t num_copy = end_pos - rec_pos;

    memcpy(&disp_samples[rec_pos], &buffer, num_copy * sizeof(i16));

    rec_pos = (rec_pos + num_copy) % DISP_SAMPLES;

    if (num_copy < num_samples)
    {
        size_t buf_pos = num_copy;
        size_t num_copy = num_samples - buf_pos;

        memcpy(&disp_samples, &buffer[buf_pos], num_copy * sizeof(i16));
        rec_pos = num_copy;
    }

    //printf("%d\n", rec_pos);
}

void main()
{
    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Audio Input Graph", 0);
    audio_open_input(SAMPLE_RATE, 1, AUDIO_FORMAT_I16, audio_cb);

    anim_event_loop(30, update);
}
