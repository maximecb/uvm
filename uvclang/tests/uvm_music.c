// Self-checking test for <uvm/music.h> (MIDI note -> frequency, f32). Runs under
// run_uvm_tests.sh: exits 0 iff every check passes. Frequencies go through powf,
// so they use a tolerance; a volatile seed keeps -O2 from const-folding calls away.
#include <assert.h>
#include <string.h>
#define UVM_MUSIC_IMPLEMENTATION
#include <uvm/music.h>

static int approx(float a, float b) { return fabsf(a - b) < 1e-2f; }

int main()
{
    // MIDI clock macro (exercises MIDI_CLOCK_PPS = MIDI_CLOCK_PPQ / 4).
    assert(MIDI_CLOCK_PPS == 6);

    // Pitch-class -> note-name table (global array of string pointers).
    volatile int pc = 9;                // opaque to the optimizer
    assert(strcmp(PC_NOTE_NAME[pc], "A") == 0);
    assert(strcmp(PC_NOTE_NAME[0], "C") == 0);
    assert(strcmp(PC_NOTE_NAME[1], "C#") == 0);
    assert(PC_NOTE_NAME[11][0] == 'B');

    // C4 sits 9 semitones below A4 (69).
    assert(C4_NOTE_NO == 60);
    assert(A4_NOTE_NO - C4_NOTE_NO == 9);

    volatile int seed = 0;
    unsigned int a4 = 69 + seed;        // 69, opaque to the optimizer

    // A4 = 440 Hz (the exponent is 0).
    assert(approx(pc_to_freq(a4, 0.0f), 440.0f));

    // One octave up / down (exercises the signed note offset).
    assert(approx(pc_to_freq(a4 + 12, 0.0f), 880.0f));   // A5
    assert(approx(pc_to_freq(a4 - 12, 0.0f), 220.0f));   // A3, below A4

    // A detuning offset of +1200 cents is one octave up.
    assert(approx(pc_to_freq(a4, 1200.0f), 880.0f));

    return 0;
}
