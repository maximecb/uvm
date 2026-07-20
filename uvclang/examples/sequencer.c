// A 16-step, 6-row pentatonic step sequencer. Click cells to toggle notes; a
// square-wave voice plays each active step in time.

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <uvm/syscalls.h>
#include <uvm/window.h>
#include <uvm/graphics.h>

#define FRAME_WIDTH 435
#define FRAME_HEIGHT 185

#define NUM_STEPS 16
#define NUM_ROWS 6

#define CELL_SIZE 20
#define PAD_SIZE 5
#define BORDER_SIZE 20

// Frame buffer to draw into
uint32_t frame_buffer[FRAME_HEIGHT][FRAME_WIDTH];

// Sequencer grid
bool grid[NUM_ROWS][NUM_STEPS];

// Buffer used for audio output
int16_t audio_buffer[1024];

// Index of the current step
uint32_t step_idx = 0;

// Sample index in the current step
uint32_t sample_idx = 0;

// Frequencies for musical notes on the pentatonic scale
// Notes at the top (lowest index) have the highest frequency
float NOTE_FREQS[6] = {
    330.0f,
    294.0f,
    261.0f,
    220.0f,
    196.0f,
    147.0f,
};

void redraw(void)
{
    // Clear the frame buffer, set all pixels to black
    memset(frame_buffer, 0, sizeof(frame_buffer));

    for (int j = 0; j < NUM_ROWS; ++j)
    {
        for (int i = 0; i < NUM_STEPS; ++i)
        {
            bool cell_on = grid[j][i];

            uint32_t color = 0x222222;
            if (cell_on)
            {
                color = (i == (int)step_idx) ? COLOR_WHITE : COLOR_RED;
            }

            fill_rect(
                (uint32_t *)frame_buffer,
                FRAME_WIDTH,
                FRAME_HEIGHT,
                BORDER_SIZE + i * (CELL_SIZE + PAD_SIZE),
                BORDER_SIZE + j * (CELL_SIZE + PAD_SIZE),
                CELL_SIZE,
                CELL_SIZE,
                color
            );
        }
    }

    window_draw_frame(0, (uint8_t *)frame_buffer);
}

void fill_audio(uint32_t num_samples)
{
    assert(num_samples <= 1024);

    memset(audio_buffer, 0, sizeof(audio_buffer));

    uint64_t beats_per_sec = 2;
    uint64_t steps_per_beat = 4;
    uint64_t steps_per_sec = beats_per_sec * steps_per_beat;
    uint64_t samples_per_step = 44100 / steps_per_sec;

    // For each sample to write in the audio buffer
    for (uint32_t buf_idx = 0; buf_idx < num_samples; ++buf_idx)
    {
        float out = 0.0f;

        // For each row of the sequencer
        for (int j = 0; j < NUM_ROWS; ++j)
        {
            // If there is no note at this position
            if (!grid[j][step_idx])
                continue;

            float freq = NOTE_FREQS[j];

            float phase = freq * (float)(int32_t)sample_idx / 44100.0f;
            float cycle_pos = phase - (float)(int)phase;

            // Here we assume that cycle_pos is in [0, 1[
            // Use a square wave for a retro sound
            float osc_val = (cycle_pos < 0.5f) ? 1.0f : -1.0f;

            out = out + osc_val * 0.3f;
        }

        // Decay envelope
        float env = 1.0f - (float)(int32_t)sample_idx / 12000.0f;
        if (env < 0.0f)
            env = 0.0f;

        // Convert the output to signed 16-bit samples
        audio_buffer[buf_idx] = (int16_t)(5000.0f * out * env);

        sample_idx = sample_idx + 1;

        // If it's time to move to the next step
        if (sample_idx >= samples_per_step)
        {
            // Move to the next step
            step_idx = (step_idx + 1) % NUM_STEPS;
            sample_idx = 0;
        }
    }

}

// Audio runs on its own thread, concurrently with the UI loop on main.
uint32_t audio_dev;

void *audio_thread(void *arg)
{
    (void)arg;
    for (;;)
    {
        audio_wait_output(audio_dev);
        fill_audio(1024);
        audio_write(audio_dev, audio_buffer, 1024);
    }
    return 0;
}

void mousedown(uint8_t btn_id, int32_t x, int32_t y)
{
    // Only handle left clicks
    if (btn_id != 0)
    {
        return;
    }

    uint32_t step = (x - BORDER_SIZE) / (CELL_SIZE + PAD_SIZE);
    uint32_t row = (y - BORDER_SIZE) / (CELL_SIZE + PAD_SIZE);

    if (row >= NUM_ROWS || step >= NUM_STEPS)
    {
        return;
    }

    grid[row][step] = !grid[row][step];

    redraw();
}

Event event;

int main(void)
{
    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Pentatonic Sequencer", 0);

    audio_dev = audio_open_output(44100, 1, AUDIO_FORMAT_I16);
    pthread_t audio_tid;
    pthread_create(&audio_tid, 0, audio_thread, 0);

    redraw();

    for (;;)
    {
        while (window_poll_event(&event))
        {
            if (event.kind == EVENT_QUIT)
            {
                exit(0);
            }

            if (event.kind == EVENT_KEYDOWN && event.key == KEY_ESCAPE)
            {
                exit(0);
            }

            if (event.kind == EVENT_MOUSEDOWN)
            {
                mousedown(event.button, event.x, event.y);
            }
        }

        thread_sleep(25);
        redraw();
    }

    return 0;
}
