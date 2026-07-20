// Record from the audio input and draw the last few seconds as a waveform, with
// a moving vertical bar at the current recording position.

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <uvm/syscalls.h>
#include <uvm/window.h>
#include <uvm/graphics.h>

#define FRAME_WIDTH 600
#define FRAME_HEIGHT 200
#define SAMPLE_RATE 44100
#define DISP_SAMPLES 176400 // SAMPLE_RATE * 4

// RGBA pixels
uint32_t frame_buffer[FRAME_HEIGHT][FRAME_WIDTH];

// Buffer for incoming samples
int16_t buffer[1024];

// Buffer for display
int16_t disp_samples[DISP_SAMPLES];

// Current recording position
size_t rec_pos = 0;

void update(void)
{
    // Clear the frame buffer, set all pixels to black
    memset(frame_buffer, 0, sizeof(frame_buffer));

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
            (uint32_t *)frame_buffer,
            FRAME_WIDTH,
            FRAME_HEIGHT,
            x - 1,
            prev_y,
            x,
            y,
            COLOR_RED
        );

        prev_y = y;
    }

    // Draw vertical line at recording position
    uint32_t rec_x = rec_pos * FRAME_WIDTH / DISP_SAMPLES;
    draw_line(
        (uint32_t *)frame_buffer,
        FRAME_WIDTH,
        FRAME_HEIGHT,
        rec_x,
        0,
        rec_x,
        FRAME_HEIGHT - 1,
        COLOR_WHITE
    );

    window_draw_frame(0, (uint8_t *)frame_buffer);
}

uint32_t audio_dev;

// Block for one buffer of captured samples and fold it into the display ring.
void capture_block(void)
{
    size_t num_samples = 1024;
    audio_read(audio_dev, buffer, (uint32_t)num_samples);

    // Copy the incoming samples into the display ring buffer, clamping at the
    // end of the buffer (the remainder wraps around below).
    size_t cap = sizeof(disp_samples) / sizeof(int16_t);
    size_t end_pos = rec_pos + num_samples;
    if (end_pos > cap)
        end_pos = cap;
    size_t num_copy = end_pos - rec_pos;

    memcpy(&disp_samples[rec_pos], buffer, num_copy * sizeof(int16_t));

    rec_pos = (rec_pos + num_copy) % DISP_SAMPLES;

    if (num_copy < num_samples)
    {
        size_t buf_pos = num_copy;
        size_t wrap_copy = num_samples - buf_pos;

        memcpy(disp_samples, &buffer[buf_pos], wrap_copy * sizeof(int16_t));
        rec_pos = wrap_copy;
    }
}

// Capture runs on its own thread; audio_read blocks until a buffer is ready.
void *audio_thread(void *arg)
{
    (void)arg;
    for (;;)
        capture_block();
    return 0;
}

int main(void)
{
    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Audio Input Graph", 0);
    audio_dev = audio_open_input(SAMPLE_RATE, 1, AUDIO_FORMAT_I16);
    pthread_t audio_tid;
    pthread_create(&audio_tid, 0, audio_thread, 0);

    anim_event_loop(30, update);
    return 0;
}
