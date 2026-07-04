// Tolerant, self-checking test for uvclang's printf %f support.
//
// uvclang narrows the double vararg to a 32-bit float before formatting (UVM has
// no f64 arithmetic), so the printed digits are only float-accurate and do NOT
// match native libc's double formatting byte-for-byte. This test therefore never
// compares the formatted text directly: it formats into a buffer, parses the
// number back out with a small float parser, and checks it against the expected
// value within a tolerance. It also exercises the float->double vararg promotion
// (fpext) at each call site and the double->float narrowing (fptrunc) inside
// printf. stdout is a single fixed line, so the differential harness still
// matches native exactly.
//
// Values are funneled through a file-scope `volatile` so -O2 can't fold the
// checks into compile-time constants — the va_arg walk must run for real.

#include <stdio.h>
#include <string.h>
#include <assert.h>

// Minimal decimal parser (sign, integer part, optional ".frac"; no exponent),
// tolerant of the leading spaces field-width padding can add. Uses float
// arithmetic (UVM has no f64), which is plenty to check float-accurate output.
static float parse_f(const char *s)
{
    while (*s == ' ')
        s++;
    int neg = 0;
    if (*s == '-')      { neg = 1; s++; }
    else if (*s == '+') s++;

    float v = 0.0f;
    while (*s >= '0' && *s <= '9') { v = v * 10.0f + (float)(*s - '0'); s++; }
    if (*s == '.') {
        s++;
        float scale = 0.1f;
        while (*s >= '0' && *s <= '9') {
            v += (float)(*s - '0') * scale;
            scale *= 0.1f;
            s++;
        }
    }
    return neg ? -v : v;
}

// Relative+absolute tolerance sized for 32-bit float precision (~7 digits).
static int close(float got, float want)
{
    float d = got - want;
    if (d < 0.0f) d = -d;
    float a = want < 0.0f ? -want : want;
    return d <= 1e-3f * (a + 1.0f);
}

// Funnel that defeats constant-folding: the format arg is read from volatile.
static volatile double VD;

// Format VD (= value) with fmt, parse the result back, compare to expect.
static int check(const char *fmt, double value, float expect)
{
    char buf[64];
    VD = value;
    snprintf(buf, sizeof buf, fmt, VD);
    return close(parse_f(buf), expect);
}

int main()
{
    char buf[64];

    // --- default precision (6 places) ---
    assert(check("%f", 3.14159, 3.14159f));
    assert(check("%f", 0.0, 0.0f));
    assert(check("%f", 1.0, 1.0f));
    assert(check("%f", 123.456, 123.456f));
    assert(check("%f", -7.25, -7.25f));
    assert(check("%f", 1000.5, 1000.5f));

    // --- explicit precision (avoid exact .5 half-way cases: native rounds to
    // even, our formatter rounds half up, so only non-boundary values agree) ---
    assert(check("%.2f", 3.14159, 3.14f));
    assert(check("%.0f", 7.3, 7.0f));
    assert(check("%.0f", 2.8, 3.0f));
    assert(check("%.4f", 0.1, 0.1f));
    assert(check("%.10f", 1.0 / 3.0, 0.3333333f));   // float-accurate ~0.333333

    // --- field width / flags: the value must survive AND the layout is exact
    // (width, padding, sign are controlled by us, identical on both sides) ---
    VD = 3.5;
    snprintf(buf, sizeof buf, "%10.2f", VD);
    assert(close(parse_f(buf), 3.5f));
    assert(strlen(buf) == 10);                 // right-justified to width 10

    snprintf(buf, sizeof buf, "%-10.2f|", VD);
    assert(close(parse_f(buf), 3.5f));
    assert(buf[0] == '3');                      // left-justified: digits first
    assert(buf[10] == '|');                     // padded out to width 10

    snprintf(buf, sizeof buf, "%08.2f", VD);
    assert(close(parse_f(buf), 3.5f));
    assert(strlen(buf) == 8);
    assert(buf[0] == '0');                      // zero-padded

    snprintf(buf, sizeof buf, "%+.2f", VD);
    assert(buf[0] == '+');                      // forced sign
    assert(close(parse_f(buf), 3.5f));

    VD = -3.5;
    snprintf(buf, sizeof buf, "% .2f", VD);
    assert(buf[0] == '-');                      // negative overrides ' '
    assert(close(parse_f(buf), -3.5f));

    // --- '*' width and precision taken from int args before the double ---
    VD = 2.71828;
    snprintf(buf, sizeof buf, "%*.*f", 9, 3, VD);
    assert(close(parse_f(buf), 2.71828f));
    assert(strlen(buf) == 9);

    // --- the walk must stay in sync: integer conversions around a %f still read
    // the right slots (this is the whole reason we go through the GP va_arg path) ---
    VD = 2.5;
    snprintf(buf, sizeof buf, "%d %f %d", 10, VD, 20);
    {
        // first int
        const char *p = buf;
        int a = 0;
        while (*p >= '0' && *p <= '9') { a = a * 10 + (*p - '0'); p++; }
        assert(a == 10);
        assert(*p == ' ');
        p++;
        // middle float
        assert(close(parse_f(p), 2.5f));
        while (*p != ' ') p++;      // skip the float token
        p++;
        // trailing int
        int b = 0;
        while (*p >= '0' && *p <= '9') { b = b * 10 + (*p - '0'); p++; }
        assert(b == 20);
    }

    // --- inf / nan: fixed spellings, checked loosely (not by exact string) ---
    union { unsigned long u; double d; } sv;
    sv.u = 0x7FF0000000000000UL;                 // +inf
    VD = sv.d;
    snprintf(buf, sizeof buf, "%f", VD);
    assert(buf[0] == 'i' && buf[1] == 'n' && buf[2] == 'f');
    sv.u = 0x7FF8000000000000UL;                 // quiet nan
    VD = sv.d;
    snprintf(buf, sizeof buf, "%f", VD);
    assert(buf[0] == 'n' && buf[1] == 'a' && buf[2] == 'n');

    puts("printf f ok");
    return 0;
}
