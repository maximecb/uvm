// Play a rising scale of eight notes using a sawtooth oscillator.

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <uvm/syscalls.h>

// Frequencies for musical notes
float NOTE_FREQS[8] = {
    130.813f, // C3
    146.832f, // D3
    164.814f, // E3
    174.614f, // F3
    195.998f, // G3
    220.000f, // A3
    246.942f, // B3
    261.626f, // C4
};

// Buffer used for audio output
int16_t audio_buffer[1024];

// Index of the current sample
size_t sample_idx = 0;

// Oscillator phase
float phase = 0.0f;

// Fill audio_buffer with `num_samples` mono samples of the current note.
void fill_buffer(uint32_t num_samples)
{
    memset(audio_buffer, 0, sizeof(audio_buffer));

    // Time taken by one sample (inverse sample rate)
    float sample_time = 1.0f / 44100.0f;

    size_t note_idx = sample_idx / 30000;
    if (note_idx > 7)
        note_idx = 7;

    float freq = NOTE_FREQS[note_idx];

    for (uint32_t i = 0; i < num_samples; ++i)
    {
        // Here we assume that phase is in [0, 1[
        // The sawtooth output is in [-1, 1[
        float osc_val = -1.0f + phase * 2.0f;

        // Convert the output to signed 16-bit samples
        audio_buffer[i] = (int16_t)(osc_val * 4000.0f);

        phase = phase + sample_time * freq;

        if (phase > 1.0f)
            phase = phase - 1.0f;
    }

    sample_idx = sample_idx + num_samples;
}

int main(void)
{
    uint32_t dev = audio_open_output(44100, 1, AUDIO_FORMAT_I16);

    // Play ~8 seconds of audio, one 1024-frame buffer at a time. The audio loop
    // runs right here on the main thread: audio_wait_output blocks until the
    // device needs the next buffer, then we synthesize it and hand it over.
    while (sample_idx < 44100 * 8)
    {
        audio_wait_output(dev);
        fill_buffer(1024);
        audio_write(dev, audio_buffer, 1024);
    }

    audio_close(dev);
    return 0;
}
