// synth.c — a small subtractive synthesizer with a 16-step sequencer,
// self-contained in this one file, for the uvclang/UVM target.
//
// The synth has 8 voices, one per sequencer row. Rows are mapped to an
// A minor pentatonic scale (A2 to D4). Each voice is a single oscillator
// (saw / square / pulse / triangle) shaped by its own ADSR amplitude
// envelope. The voice mix runs through a drive stage (up to 64x of gain into
// an asymmetric hard clipper, followed by a DC blocker), then a resonant
// low-pass filter (Chamberlin state-variable, run at 2x the sample rate so
// it stays stable with the cutoff opened all the way), then the master
// volume.
//
// The audio callback runs on its own VM thread and reads the knob and grid
// state written by the UI thread. All shared values are single words, so the
// unsynchronized accesses are benign (same pattern as the ncc audio examples).
//
// Controls:
//   - Knobs: click a knob and drag the mouse up/down to turn it.
//     Hold SHIFT while dragging for fine adjustment.
//   - Grid: click a cell to toggle a step. The playhead column lights up.
//   - SPACE toggles play/stop. ESC quits.
//
// Build and run from the repo root:
//   uvclang/target/debug/uvclang -O2 uvclang/examples/synth.c -o synth.asm
//   vm/target/debug/uvm synth.asm

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <uvm/syscalls.h>
#include <uvm/graphics.h>
#include <uvm/window.h>
#include <uvm/music.h>

//-----------------------------------------------------------------------------
// Layout and audio constants
//-----------------------------------------------------------------------------

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 430

#define SAMPLE_RATE 44100
#define AUDIO_BUF_LEN 1024

// Sequencer grid geometry
#define NUM_STEPS 16
#define NUM_ROWS 8
#define CELL_SIZE 28
#define CELL_PAD 6
#define CELL_PITCH (CELL_SIZE + CELL_PAD)
#define GRID_X 54
#define GRID_Y 132

// Knob geometry
#define KNOB_RADIUS 16
#define KNOB_Y 62

// UI colors (0xAARRGGBB, same encoding as rgba32 in <uvm/graphics.h>)
#define COLOR_BG        0xFF16161A
#define COLOR_KNOB_BODY 0xFF2E2E34
#define COLOR_KNOB_RING 0xFF4A4A52
#define COLOR_ACCENT    0xFFFF9020
#define COLOR_TEXT      0xFF9A9AA0
#define COLOR_TEXT_DIM  0xFF5A5A60
#define COLOR_CELL_OFF  0xFF232329
#define COLOR_CELL_OFF2 0xFF2C2C33
#define COLOR_CELL_PLAY 0xFF3C3C46

// Frame buffer to draw into
uint32_t frame_buffer[FRAME_HEIGHT][FRAME_WIDTH];

//-----------------------------------------------------------------------------
// Tiny 3x5 pixel font: digits, uppercase letters and a few symbols.
// Each glyph is 5 rows of 3 pixels, packed into one u16 (3 bits per row).
//-----------------------------------------------------------------------------

// Pack five 3-bit rows into a glyph, one octal digit per row.
#define G(r0, r1, r2, r3, r4) \
    (uint16_t)(((r0) << 12) | ((r1) << 9) | ((r2) << 6) | ((r3) << 3) | (r4))

// Glyphs for 0-9, then A-Z, then # . % / -
const uint16_t FONT[41] = {
    G(7,5,5,5,7), // 0
    G(2,6,2,2,7), // 1
    G(7,1,7,4,7), // 2
    G(7,1,3,1,7), // 3
    G(5,5,7,1,1), // 4
    G(7,4,7,1,7), // 5
    G(7,4,7,5,7), // 6
    G(7,1,1,2,2), // 7
    G(7,5,7,5,7), // 8
    G(7,5,7,1,7), // 9
    G(2,5,7,5,5), // A
    G(6,5,6,5,6), // B
    G(3,4,4,4,3), // C
    G(6,5,5,5,6), // D
    G(7,4,6,4,7), // E
    G(7,4,6,4,4), // F
    G(3,4,5,5,3), // G
    G(5,5,7,5,5), // H
    G(7,2,2,2,7), // I
    G(1,1,1,5,2), // J
    G(5,5,6,5,5), // K
    G(4,4,4,4,7), // L
    G(5,7,7,5,5), // M
    G(6,5,5,5,5), // N
    G(2,5,5,5,2), // O
    G(7,5,7,4,4), // P
    G(7,5,5,7,1), // Q
    G(6,5,6,5,5), // R
    G(3,4,2,1,6), // S
    G(7,2,2,2,2), // T
    G(5,5,5,5,7), // U
    G(5,5,5,5,2), // V
    G(5,5,7,7,5), // W
    G(5,5,2,5,5), // X
    G(5,5,2,2,2), // Y
    G(7,1,2,4,7), // Z
    G(5,7,5,7,5), // #
    G(0,0,0,0,2), // .
    G(5,1,2,4,5), // %
    G(1,1,2,4,4), // /
    G(0,0,7,0,0), // -
};

#define FONT_SCALE 2
#define GLYPH_H (5 * FONT_SCALE)
#define CHAR_ADVANCE (4 * FONT_SCALE)

// Map a character to its glyph index, or -1 for characters drawn as blanks.
int32_t glyph_index(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'Z')
        return 10 + (c - 'A');
    if (c == '#')
        return 36;
    if (c == '.')
        return 37;
    if (c == '%')
        return 38;
    if (c == '/')
        return 39;
    if (c == '-')
        return 40;
    return -1;
}

void draw_char(char c, uint32_t x, uint32_t y, uint32_t color)
{
    int32_t idx = glyph_index(c);
    if (idx < 0)
        return;

    uint16_t bits = FONT[idx];

    for (uint32_t row = 0; row < 5; ++row)
    {
        uint32_t row_bits = ((uint32_t)bits >> (12 - row * 3)) & 7;

        for (uint32_t col = 0; col < 3; ++col)
        {
            if ((row_bits >> (2 - col)) & 1)
            {
                fill_rect(
                    (uint32_t*)frame_buffer,
                    FRAME_WIDTH,
                    FRAME_HEIGHT,
                    x + col * FONT_SCALE,
                    y + row * FONT_SCALE,
                    FONT_SCALE,
                    FONT_SCALE,
                    color
                );
            }
        }
    }
}

void draw_text(const char* str, uint32_t x, uint32_t y, uint32_t color)
{
    for (uint32_t i = 0; str[i]; ++i)
    {
        draw_char(str[i], x + i * CHAR_ADVANCE, y, color);
    }
}

uint32_t text_width(const char* str)
{
    uint32_t n = 0;
    while (str[n])
        ++n;
    return (n > 0)? (n * CHAR_ADVANCE - FONT_SCALE):0;
}

// Draw text horizontally centered on cx.
void draw_text_centered(const char* str, uint32_t cx, uint32_t y, uint32_t color)
{
    draw_text(str, cx - text_width(str) / 2, y, color);
}

//-----------------------------------------------------------------------------
// Knobs
//-----------------------------------------------------------------------------

// Rotary controls: click a knob, then drag the mouse vertically to turn it
// (dragging up turns clockwise). Each knob stores a normalized position in
// [0, 1] which is mapped to its actual parameter range where it is used.

enum
{
    KNOB_WAVE,
    KNOB_ATK,
    KNOB_DEC,
    KNOB_SUS,
    KNOB_REL,
    KNOB_CUT,
    KNOB_RES,
    KNOB_DRV,
    KNOB_BPM,
    KNOB_VOL,
    NUM_KNOBS
};

typedef struct
{
    const char* label;
    uint32_t x;     // center x position (all knobs sit on the KNOB_Y line)
    float value;    // normalized position in [0, 1]
} Knob;

Knob knobs[NUM_KNOBS] = {
    { "WAVE",  46, 0.00f },
    { "ATK",  116, 0.10f },
    { "DEC",  172, 0.50f },
    { "SUS",  228, 0.60f },
    { "REL",  284, 0.40f },
    { "CUT",  354, 0.75f },
    { "RES",  410, 0.30f },
    { "DRV",  480, 0.25f },
    { "BPM",  550, 0.43f },
    { "VOL",  606, 0.70f },
};

const char* const WAVE_NAMES[4] = { "SAW", "SQR", "PLS", "TRI" };

// Index of the knob being dragged, or -1
int32_t drag_knob = -1;

// Last mouse y position seen during a knob drag
int32_t drag_last_y = 0;

// Whether the shift key is held (fine knob adjustment)
bool shift_down = false;

// Map an envelope time knob to seconds (1ms to 2s, exponential).
float knob_time(uint32_t idx)
{
    return 0.001f * powf(2000.0f, knobs[idx].value);
}

// Filter cutoff frequency in Hz (80Hz to 14kHz, exponential).
float knob_cutoff(void)
{
    return 80.0f * powf(175.0f, knobs[KNOB_CUT].value);
}

// Distortion pre-gain (1x to 64x, exponential).
float knob_drive(void)
{
    return powf(64.0f, knobs[KNOB_DRV].value);
}

// Tempo in beats per minute (60 to 200).
float knob_bpm(void)
{
    return 60.0f + 140.0f * knobs[KNOB_BPM].value;
}

// Oscillator waveform index (the wave knob snaps to 4 positions).
uint32_t knob_wave(void)
{
    return (uint32_t)(knobs[KNOB_WAVE].value * 3.99f);
}

//-----------------------------------------------------------------------------
// Sequencer and synth voices
//-----------------------------------------------------------------------------

// MIDI note numbers for the grid rows, top row first (A minor pentatonic)
const uint8_t ROW_NOTES[NUM_ROWS] = { 62, 60, 57, 55, 52, 50, 48, 45 };

// Note names for the row labels
const char* const ROW_NAMES[NUM_ROWS] = {
    "D4", "C4", "A3", "G3", "E3", "D3", "C3", "A2"
};

// Sequencer grid; grid[row][step] is true when the note is on
bool grid[NUM_ROWS][NUM_STEPS];

// Amplitude envelope stages
#define ENV_IDLE 0
#define ENV_ATTACK 1
#define ENV_DECAY 2
#define ENV_RELEASE 3

// One synth voice per grid row
typedef struct
{
    float freq;      // oscillator frequency in Hz
    float phase;     // oscillator phase in [0, 1)
    float env;       // envelope output level
    uint32_t stage;  // current envelope stage
} Voice;

Voice voices[NUM_ROWS];

// Whether the sequencer is running
bool playing = true;

// Current step (written by the audio thread, read by the UI thread)
uint32_t step_idx = 0;

// Sample position within the current step
uint32_t step_sample = 0;

// Low-pass filter state
float flt_low = 0.0f;
float flt_band = 0.0f;

// DC blocker state for the drive stage
float flt_dc = 0.0f;

// Buffer used for audio output
int16_t audio_buffer[AUDIO_BUF_LEN];

// Convert an envelope stage time to a one-pole smoothing coefficient:
// 1 - e^(-1/T), computed as 1 - 2^(-log2(e)/T) since UVM only has exp2.
float env_coef(float time_secs)
{
    return 1.0f - exp2f(-1.4427f / (time_secs * (float)SAMPLE_RATE));
}

// Audio callback, run by the VM on a dedicated audio thread
int16_t* audio_cb(uint64_t num_channels, uint64_t num_samples)
{
    assert(num_channels == 1);
    assert(num_samples <= AUDIO_BUF_LEN);

    // Read the knobs once per block
    float atk_coef = env_coef(knob_time(KNOB_ATK));
    float dec_coef = env_coef(knob_time(KNOB_DEC));
    float rel_coef = env_coef(knob_time(KNOB_REL));
    float sustain = knobs[KNOB_SUS].value;
    uint32_t wave = knob_wave();
    float drive = knob_drive();
    float volume = knobs[KNOB_VOL].value;
    volume = volume * volume;

    // State-variable filter coefficients. The filter section runs two
    // half-steps per output sample (2x oversampling), which keeps it stable
    // all the way up to the 14kHz cutoff limit.
    float f = 2.0f * sinf(M_PI_F * knob_cutoff() / (2.0f * SAMPLE_RATE));
    float damp = 1.0f - 0.9f * knobs[KNOB_RES].value;

    // Sixteenth note length in samples: (60 / (bpm * 4)) * sample_rate
    uint32_t samples_per_step = (uint32_t)(SAMPLE_RATE * 15.0f / knob_bpm());
    uint32_t gate_len = samples_per_step * 3 / 5;

    // When stopped, put all held voices into release
    if (!playing)
    {
        for (uint32_t r = 0; r < NUM_ROWS; ++r)
        {
            if (voices[r].stage == ENV_ATTACK || voices[r].stage == ENV_DECAY)
                voices[r].stage = ENV_RELEASE;
        }
    }

    for (uint32_t i = 0; i < num_samples; ++i)
    {
        if (playing)
        {
            // At the start of a step, trigger the voices for its active cells
            if (step_sample == 0)
            {
                for (uint32_t r = 0; r < NUM_ROWS; ++r)
                {
                    if (grid[r][step_idx])
                    {
                        voices[r].stage = ENV_ATTACK;
                        voices[r].phase = 0.0f;
                    }
                }
            }

            // Past the end of the gate, release whatever is still held.
            // Idempotent, so it stays correct if the tempo knob moves mid-step.
            if (step_sample >= gate_len)
            {
                for (uint32_t r = 0; r < NUM_ROWS; ++r)
                {
                    if (voices[r].stage == ENV_ATTACK || voices[r].stage == ENV_DECAY)
                        voices[r].stage = ENV_RELEASE;
                }
            }

            ++step_sample;
            if (step_sample >= samples_per_step)
            {
                step_sample = 0;
                step_idx = (step_idx + 1) % NUM_STEPS;
            }
        }

        // Run the envelopes and oscillators and mix the voices
        float mix = 0.0f;

        for (uint32_t r = 0; r < NUM_ROWS; ++r)
        {
            Voice* v = &voices[r];

            if (v->stage == ENV_IDLE)
                continue;

            if (v->stage == ENV_ATTACK)
            {
                // Aim above 1 so the attack reaches full level in finite time
                v->env = v->env + atk_coef * (1.25f - v->env);
                if (v->env >= 1.0f)
                {
                    v->env = 1.0f;
                    v->stage = ENV_DECAY;
                }
            }
            else if (v->stage == ENV_DECAY)
            {
                // Decay toward the sustain level and hold there
                v->env = v->env + dec_coef * (sustain - v->env);
            }
            else
            {
                v->env = v->env - rel_coef * v->env;
                if (v->env < 0.002f)
                {
                    v->env = 0.0f;
                    v->stage = ENV_IDLE;
                    continue;
                }
            }

            // Advance the oscillator phase
            v->phase = v->phase + v->freq * (1.0f / SAMPLE_RATE);
            if (v->phase >= 1.0f)
                v->phase = v->phase - 1.0f;

            float osc;
            if (wave == 0)
                osc = 2.0f * v->phase - 1.0f;                   // sawtooth
            else if (wave == 1)
                osc = (v->phase < 0.5f)? 1.0f:-1.0f;            // square
            else if (wave == 2)
                osc = (v->phase < 0.25f)? 1.0f:-1.0f;           // 25% pulse
            else
                osc = (v->phase < 0.5f)?                        // triangle
                    (4.0f * v->phase - 1.0f):(3.0f - 4.0f * v->phase);

            mix = mix + osc * v->env;
        }

        mix = mix * 0.2f;

        // Drive stage: pre-gain into a hard clipper. The clip thresholds are
        // asymmetric (the positive half clips sooner), which adds even
        // harmonics on top of the odd ones and makes the crunch thicker.
        float x = mix * drive;
        if (x > 0.8f)
            x = 0.8f;
        else if (x < -1.0f)
            x = -1.0f;

        // Remove the DC offset the asymmetric clipping leaves behind
        // (one-pole blocker at ~15Hz), and level-compensate
        flt_dc = flt_dc + 0.002f * (x - flt_dc);
        x = (x - flt_dc) * 0.85f;

        // Resonant low-pass filter, two half-rate steps per sample
        flt_low = flt_low + f * flt_band;
        float high = x - flt_low - damp * flt_band;
        flt_band = flt_band + f * high;

        flt_low = flt_low + f * flt_band;
        high = x - flt_low - damp * flt_band;
        flt_band = flt_band + f * high;

        float out = flt_low * volume;

        if (out > 1.0f)
            out = 1.0f;
        else if (out < -1.0f)
            out = -1.0f;

        audio_buffer[i] = (int16_t)(out * 30000.0f);
    }

    return audio_buffer;
}

//-----------------------------------------------------------------------------
// Drawing
//-----------------------------------------------------------------------------

// Fill a disc centered at (cx, cy)
void draw_disc(int32_t cx, int32_t cy, int32_t r, uint32_t color)
{
    for (int32_t dy = -r; dy <= r; ++dy)
    {
        for (int32_t dx = -r; dx <= r; ++dx)
        {
            if (dx * dx + dy * dy > r * r)
                continue;

            int32_t x = cx + dx;
            int32_t y = cy + dy;

            if (x >= 0 && x < FRAME_WIDTH && y >= 0 && y < FRAME_HEIGHT)
                frame_buffer[y][x] = color;
        }
    }
}

// Format a knob's current value for display
void knob_value_str(uint32_t idx, char* buf)
{
    if (idx == KNOB_WAVE)
    {
        snprintf(buf, 16, "%s", WAVE_NAMES[knob_wave()]);
    }
    else if (idx == KNOB_ATK || idx == KNOB_DEC || idx == KNOB_REL)
    {
        float t = knob_time(idx);
        if (t < 1.0f)
            snprintf(buf, 16, "%d MS", (int)(t * 1000.0f + 0.5f));
        else
            snprintf(buf, 16, "%.2f S", t);
    }
    else if (idx == KNOB_CUT)
    {
        float fc = knob_cutoff();
        if (fc < 1000.0f)
            snprintf(buf, 16, "%d HZ", (int)(fc + 0.5f));
        else
            snprintf(buf, 16, "%.1f KHZ", fc / 1000.0f);
    }
    else if (idx == KNOB_DRV)
    {
        snprintf(buf, 16, "%.1fX", knob_drive());
    }
    else if (idx == KNOB_BPM)
    {
        snprintf(buf, 16, "%d", (int)(knob_bpm() + 0.5f));
    }
    else
    {
        // SUS, RES and VOL show as percentages
        snprintf(buf, 16, "%d%%", (int)(knobs[idx].value * 100.0f + 0.5f));
    }
}

void draw_knob(uint32_t idx)
{
    Knob* k = &knobs[idx];

    // Stepped knobs show the snapped position
    float value = k->value;
    if (idx == KNOB_WAVE)
        value = (float)knob_wave() / 3.0f;

    // Body, with a highlighted ring while dragging
    uint32_t ring = (drag_knob == (int32_t)idx)? COLOR_ACCENT:COLOR_KNOB_RING;
    draw_disc(k->x, KNOB_Y, KNOB_RADIUS + 2, ring);
    draw_disc(k->x, KNOB_Y, KNOB_RADIUS, COLOR_KNOB_BODY);

    // Needle, sweeping 270 degrees from the lower left to the lower right
    float angle = M_PI_F * (0.75f + 1.5f * value);
    int32_t nx = k->x + (int32_t)(cosf(angle) * (KNOB_RADIUS - 4));
    int32_t ny = KNOB_Y + (int32_t)(sinf(angle) * (KNOB_RADIUS - 4));

    draw_line(
        (uint32_t*)frame_buffer, FRAME_WIDTH, FRAME_HEIGHT,
        k->x, KNOB_Y, nx, ny, COLOR_WHITE
    );
    draw_disc(nx, ny, 2, COLOR_ACCENT);

    // Label above, current value below
    draw_text_centered(k->label, k->x, KNOB_Y - KNOB_RADIUS - 16, COLOR_TEXT);

    char buf[16];
    knob_value_str(idx, buf);
    draw_text_centered(buf, k->x, KNOB_Y + KNOB_RADIUS + 8, COLOR_TEXT_DIM);
}

void redraw(void)
{
    memset32((uint32_t*)frame_buffer, COLOR_BG, FRAME_WIDTH * FRAME_HEIGHT);

    for (uint32_t i = 0; i < NUM_KNOBS; ++i)
        draw_knob(i);

    // Read the playhead position once (the audio thread updates it)
    uint32_t play_col = step_idx;

    for (uint32_t r = 0; r < NUM_ROWS; ++r)
    {
        draw_text(
            ROW_NAMES[r],
            GRID_X - 22,
            GRID_Y + r * CELL_PITCH + (CELL_SIZE - GLYPH_H) / 2,
            COLOR_TEXT_DIM
        );

        for (uint32_t s = 0; s < NUM_STEPS; ++s)
        {
            uint32_t color;
            if (grid[r][s])
                color = (playing && s == play_col)? COLOR_WHITE:COLOR_ACCENT;
            else if (playing && s == play_col)
                color = COLOR_CELL_PLAY;
            else
                // Alternate the background every four steps to mark the beats
                color = ((s / 4) % 2 == 0)? COLOR_CELL_OFF2:COLOR_CELL_OFF;

            fill_rect(
                (uint32_t*)frame_buffer, FRAME_WIDTH, FRAME_HEIGHT,
                GRID_X + s * CELL_PITCH,
                GRID_Y + r * CELL_PITCH,
                CELL_SIZE, CELL_SIZE, color
            );
        }
    }

    draw_text(
        "SPACE PLAY/STOP    CLICK STEPS TO TOGGLE    DRAG KNOBS UP/DOWN",
        GRID_X, FRAME_HEIGHT - 20, COLOR_TEXT_DIM
    );

    window_draw_frame(0, (const uint8_t*)frame_buffer);
}

//-----------------------------------------------------------------------------
// Input handling
//-----------------------------------------------------------------------------

void handle_mousedown(uint16_t button, int32_t x, int32_t y)
{
    // Only handle left clicks
    if (button != 0)
        return;

    // Check for a click on a knob
    for (uint32_t i = 0; i < NUM_KNOBS; ++i)
    {
        int32_t dx = x - (int32_t)knobs[i].x;
        int32_t dy = y - KNOB_Y;

        if (dx * dx + dy * dy <= (KNOB_RADIUS + 4) * (KNOB_RADIUS + 4))
        {
            drag_knob = (int32_t)i;
            drag_last_y = y;
            return;
        }
    }

    // Check for a click on a sequencer cell
    if (x >= GRID_X && y >= GRID_Y)
    {
        uint32_t s = (uint32_t)(x - GRID_X) / CELL_PITCH;
        uint32_t r = (uint32_t)(y - GRID_Y) / CELL_PITCH;

        if (s < NUM_STEPS && r < NUM_ROWS)
            grid[r][s] = !grid[r][s];
    }
}

void handle_mousemove(int32_t x, int32_t y)
{
    if (drag_knob < 0)
        return;

    // Dragging up turns the knob clockwise; shift makes it 10x finer
    float sens = shift_down? 0.0005f:0.005f;
    float value = knobs[drag_knob].value + sens * (float)(drag_last_y - y);

    if (value < 0.0f)
        value = 0.0f;
    if (value > 1.0f)
        value = 1.0f;

    knobs[drag_knob].value = value;
    drag_last_y = y;
}

//-----------------------------------------------------------------------------
// Main event loop
//-----------------------------------------------------------------------------

// Scratch event struct
Event event;

int main(void)
{
    // Compute the voice frequencies from the row notes
    for (uint32_t r = 0; r < NUM_ROWS; ++r)
        voices[r].freq = pc_to_freq(ROW_NOTES[r], 0.0f);

    // Start with a little pattern so there is something to hear right away
    grid[7][0] = true;
    grid[4][2] = true;
    grid[2][4] = true;
    grid[3][6] = true;
    grid[1][7] = true;
    grid[7][8] = true;
    grid[4][10] = true;
    grid[1][11] = true;
    grid[0][12] = true;
    grid[3][14] = true;

    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Step Synth", 0);
    audio_open_output(SAMPLE_RATE, 1, AUDIO_FORMAT_I16, (void*)audio_cb);

    for (;;)
    {
        while (window_poll_event(&event))
        {
            if (event.kind == EVENT_QUIT)
                return 0;

            if (event.kind == EVENT_KEYDOWN)
            {
                if (event.key == KEY_ESCAPE)
                    return 0;

                if (event.key == KEY_SPACE)
                {
                    playing = !playing;
                    if (playing)
                    {
                        // Restart from the top of the pattern
                        step_sample = 0;
                        step_idx = 0;
                    }
                }

                if (event.key == KEY_SHIFT)
                    shift_down = true;
            }

            if (event.kind == EVENT_KEYUP && event.key == KEY_SHIFT)
                shift_down = false;

            if (event.kind == EVENT_MOUSEDOWN)
                handle_mousedown(event.button, event.x, event.y);

            if (event.kind == EVENT_MOUSEUP)
                drag_knob = -1;

            if (event.kind == EVENT_MOUSEMOVE)
                handle_mousemove(event.x, event.y);
        }

        redraw();

        // Update the display at about 30 frames per second
        thread_sleep(33);
    }

    return 0;
}
