#include <uvm/syscalls.h>
#include <uvm/window.h>
#include <uvm/graphics.h>
#include <stdlib.h>
#include <stdint.h>

#define FRAME_WIDTH 435
#define FRAME_HEIGHT 185

#define NUM_STEPS 16
#define NUM_ROWS 6

#define CELL_SIZE 20
#define PAD_SIZE 5
#define BORDER_SIZE 20

// Frame buffer to draw into
u32 frame_buffer[FRAME_HEIGHT][FRAME_WIDTH];

// Sequencer grid
bool grid[NUM_ROWS][NUM_STEPS];

// Buffer used for audio output
u16 audio_buffer[1024];

// Index of the current step
u32 step_idx = 0;

// Sample index in the current step
u32 sample_idx = 0;

// Frequencies for musical notes on the pentatonic scale
float NOTE_FREQS[6] = {
    330.0f,
    294.0f,
    261.0f,
    220.0f,
    196.0f,
    147.0f,
};

void redraw()
{
    // Clear the frame buffer, set all pixels to black
    memset32(frame_buffer, 0, sizeof(frame_buffer) / sizeof(u32));

    for (int j = 0; j < NUM_ROWS; ++j)
    {
        for (int i = 0; i < NUM_STEPS; ++i)
        {
            bool cell_on = grid[j][i];

            u32 color = 0x22_22_22;
            if (cell_on)
            {
                color = (i == step_idx)? COLOR_WHITE:COLOR_RED;
            }

            fill_rect(
                (u32*)frame_buffer,
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

    window_draw_frame(0, frame_buffer);
}

u16* audio_cb(u16 num_channels, u32 num_samples)
{
    assert(num_channels == 1);
    assert(num_samples <= 1024);

    memset(audio_buffer, 0, sizeof(audio_buffer));

    u64 bpm = 120;
    u64 beats_per_sec = 2;
    u64 steps_per_beat = 4;
    u64 steps_per_sec = beats_per_sec * steps_per_beat;
    u64 samples_per_step = 44100 / steps_per_sec;

    // For each sample to generate
    for (int i = 0; i < num_samples; ++i)
    {
        float out = 0.0f;

        for (int j = 0; j < NUM_ROWS; ++j)
        {
            if (!grid[j][step_idx])
                continue;

            float freq = NOTE_FREQS[j];

            float phase = freq * (float)(i32)sample_idx / 44100.0f;
            float cycle_pos = phase - (float)(int)phase;

            // Here we assume that cycle_pos is in [0, 1[
            // Use a square wave for a retro sound
            float osc_val = (cycle_pos < 0.5f)? 1.0f:-1.0f;

            // Convert the output to signed 16-bit i16 samples
            out = out + osc_val * 0.3f;
        }

        float env = 1.0f - (float)(i32)sample_idx / 12000.0f;
        if (env < 0.0f)
            env = 0.0f;

        audio_buffer[i] = (i16)(5000.0f * out * env);

        sample_idx = sample_idx + 1;

        // If it's time to move to the next step
        if (sample_idx >= samples_per_step)
        {
            // Move to the next step
            step_idx = (step_idx + 1) % NUM_STEPS;
            sample_idx = 0;

            // Schedule redrawing as soon as we are done generating audio
            time_delay_cb(0, redraw);
        }
    }

   return audio_buffer;
}

void mousedown(u64 window_id, u8 btn_id, i32 x, i32 y)
{
    if (btn_id != 0)
    {
        return;
    }

    u32 step_idx = (x - BORDER_SIZE) / (CELL_SIZE + PAD_SIZE);
    u32 cell_x = (x - BORDER_SIZE) % (CELL_SIZE + PAD_SIZE);

    u32 row_idx = (y - BORDER_SIZE) / (CELL_SIZE + PAD_SIZE);
    u32 cell_y = (y - BORDER_SIZE) % (CELL_SIZE + PAD_SIZE);

    if (row_idx >= NUM_ROWS || step_idx >= NUM_STEPS)
    {
        return;
    }

    grid[row_idx][step_idx] = !grid[row_idx][step_idx];

    redraw();
}

void main()
{
    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Pentatonic Sequencer", 0);

    audio_open_output(44100, 1, AUDIO_FORMAT_I16, audio_cb);

    redraw();






    //window_on_keydown(0, keydown);
    //window_on_mousedown(0, mousedown);



}
