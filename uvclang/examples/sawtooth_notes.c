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

int16_t *audio_cb(uint64_t num_channels, uint64_t num_samples)
{
    assert(num_channels == 1);
    assert(num_samples <= 1024);

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

    return audio_buffer;
}

int main(void)
{
    audio_open_output(44100, 1, AUDIO_FORMAT_I16, (void *)audio_cb);

    // Keep the program running until audio is done playing
    thread_sleep(8000);
    return 0;
}
