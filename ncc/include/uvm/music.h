#ifndef __UVM_MUSIC_H__
#define __UVM_MUSIC_H__

#include <assert.h>
#include <math.h>

// MIDI clock pulses per quarter note
#define MIDI_CLOCK_PPQ 24

// MIDI clock pulses per 16th note
#define MIDI_CLOCK_PPS (CLOCK_PPQ / 4)

// Number of MIDI notes
#define MIDI_NUM_NOTES 128

// Number of notes per octave
#define NOTES_PER_OCTAVE 12

// Number of cents per octave
#define CENTS_PER_OCTAVE 1200

// Frequency of the A4 note
#define A4_NOTE_FREQ 440

// Note number of the A4 note
#define A4_NOTE_NO 69

// Note number of the C4 note
#define C4_NOTE_NO 71

/*
// Mapping from pitch classes to note names
char* PC_NOTE_NAME[12] = {
    "C",    // 0
    "C#",   // 1
    "D",    // 2
    "D#",   // 3
    "E",    // 4
    "F",    // 5
    "F#",   // 6
    "G",    // 7
    "G#",   // 8
    "A",    // 9
    "A#",   // 10
    "B"     // 11
};
*/

// Get the frequency for a note
// offset is detuning offset in cents
float pc_to_freq(unsigned int note_no, float offset)
{
    assert(note_no < MIDI_NUM_NOTES);

    // F(n) = 440 * 2 ^ ((n-69)/12)
    float note_exp = (float)(note_no - A4_NOTE_NO) / NOTES_PER_OCTAVE;

    // b = a * 2 ^ (o / 1200)
    float offset_exp = offset / CENTS_PER_OCTAVE;

    // Compute the note frequency
    return A4_NOTE_FREQ * powf(
        2,
        note_exp + offset_exp
    );
}

#endif
