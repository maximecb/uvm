// Exact, self-checking test for uvclang's printf floating-point support.
//
// UVM has f64 arithmetic, so printf formats doubles with full precision and the
// output matches native libc byte-for-byte across the normal range. This test
// therefore compares the formatted text *directly* (strcmp) against the exact
// strings native libc produces — the expected literals below were generated with
// the platform's own printf. It covers %f/%e/%g/%a, ties-to-even rounding, the
// fixed/scientific selection of %g, and hex floats, plus the field-width/flag
// layout and the va_arg walk staying in sync around a double.
//
// Every value goes through a file-scope `volatile` so -O2 cannot fold the call
// into a compile-time-formatted string constant — the va_arg walk must run for
// real, exercising uvclang's formatter rather than clang's.
//
// The chosen values stay within ~15 significant digits and |x| < 2^63, where the
// f64 formatter is exact; higher precision and astronomically large %f magnitudes
// are subject to a documented ~17-digit ceiling and are intentionally not tested
// for byte-exactness here.

#include <stdio.h>
#include <string.h>
#include <assert.h>

// Funnel that defeats constant-folding: the format arg is read from volatile.
static volatile double VD;

// Format `value` with `fmt` and compare the result to `expect` exactly.
static int eq(const char *fmt, double value, const char *expect)
{
    char buf[128];
    VD = value;
    snprintf(buf, sizeof buf, fmt, VD);
    return strcmp(buf, expect) == 0;
}

int main()
{
    char buf[128];

    // --- %f: default precision, explicit precision, sign, ties-to-even ---
    assert(eq("%f", 3.14159, "3.141590"));
    assert(eq("%f", 0.0, "0.000000"));
    assert(eq("%f", 1.0, "1.000000"));
    assert(eq("%f", 123.456, "123.456000"));
    assert(eq("%f", -7.25, "-7.250000"));
    assert(eq("%f", 1000.5, "1000.500000"));
    assert(eq("%f", 100.7, "100.700000"));
    assert(eq("%.2f", 3.14159, "3.14"));
    assert(eq("%.0f", 7.3, "7"));
    assert(eq("%.0f", 2.8, "3"));
    assert(eq("%.0f", 0.5, "0"));       // tie -> even (0)
    assert(eq("%.0f", 1.5, "2"));       // tie -> even (2)
    assert(eq("%.0f", 2.5, "2"));       // tie -> even (2)
    assert(eq("%.2f", 0.125, "0.12"));  // tie -> even (2)
    assert(eq("%.4f", 0.1, "0.1000"));
    assert(eq("%.10f", 1.0 / 3.0, "0.3333333333"));
    assert(eq("%.3f", 9.9995, "9.999"));
    assert(eq("%f", -0.0, "-0.000000"));

    // --- %e / %E ---
    assert(eq("%e", 0.0, "0.000000e+00"));
    assert(eq("%e", 12345.678, "1.234568e+04"));
    assert(eq("%e", 1.0, "1.000000e+00"));
    assert(eq("%e", -4.56e-5, "-4.560000e-05"));
    assert(eq("%.0e", 5.0, "5e+00"));
    assert(eq("%.3e", 2.5, "2.500e+00"));
    assert(eq("%E", 6.022e23, "6.022000E+23"));
    assert(eq("%.2e", 9.999e9, "1.00e+10"));      // carry ripples into exponent
    assert(eq("%e", 0.0001, "1.000000e-04"));

    // --- %g / %G: trailing-zero trim and the fixed-vs-scientific rule ---
    assert(eq("%g", 0.0, "0"));
    assert(eq("%g", 100000.0, "100000"));         // exp 5 < 6 -> fixed
    assert(eq("%g", 1000000.0, "1e+06"));         // exp 6 >= 6 -> scientific
    assert(eq("%g", 0.0001234, "0.0001234"));     // exp -4 -> fixed
    assert(eq("%g", 0.00001234, "1.234e-05"));    // exp -5 -> scientific
    assert(eq("%g", 3.14159, "3.14159"));
    assert(eq("%.3g", 2.5, "2.5"));
    assert(eq("%.10g", 1.0 / 3.0, "0.3333333333"));
    assert(eq("%G", 1.5e-10, "1.5E-10"));
    assert(eq("%g", 123456.0, "123456"));
    assert(eq("%g", 0.1, "0.1"));
    assert(eq("%#g", 1.5, "1.50000"));            // '#' keeps trailing zeros
    assert(eq("%g", 42.0, "42"));

    // --- %a / %A: exact hex floats, incl. rounding and negative ---
    assert(eq("%a", 1.0, "0x1p+0"));
    assert(eq("%a", 3.0, "0x1.8p+1"));
    assert(eq("%a", 0.5, "0x1p-1"));
    assert(eq("%a", 0.0, "0x0p+0"));
    assert(eq("%a", 2.0, "0x1p+1"));
    assert(eq("%a", 0.1, "0x1.999999999999ap-4"));
    assert(eq("%.3a", 1.0 / 3.0, "0x1.555p-2"));
    assert(eq("%a", -1.5, "-0x1.8p+0"));
    assert(eq("%A", 256.0, "0X1P+8"));

    // --- field width / flags: layout is controlled by us and exact ---
    assert(eq("%10.2f", 3.5, "      3.50"));
    assert(eq("%-10.2f", 3.5, "3.50      "));
    assert(eq("%08.2f", 3.5, "00003.50"));
    assert(eq("%+.2f", 3.5, "+3.50"));
    assert(eq("% .2f", -3.5, "-3.50"));           // negative overrides ' '
    assert(eq("%12.3e", 2.5, "   2.500e+00"));
    assert(eq("%-12.3g", 100.0, "100         "));

    // --- '*' width and precision taken from int args before the double ---
    VD = 2.71828;
    snprintf(buf, sizeof buf, "%*.*f", 9, 3, VD);
    assert(strcmp(buf, "    2.718") == 0);

    // --- the walk must stay in sync: integer conversions around a %f still read
    // the right slots (the whole reason we go through the GP va_arg path) ---
    VD = 2.5;
    snprintf(buf, sizeof buf, "%d %f %d", 10, VD, 20);
    assert(strcmp(buf, "10 2.500000 20") == 0);

    // --- inf / nan spellings ---
    union { unsigned long u; double d; } sv;
    sv.u = 0x7FF0000000000000UL;                  // +inf
    VD = sv.d;
    snprintf(buf, sizeof buf, "%f|%e|%g", VD, VD, VD);
    assert(strcmp(buf, "inf|inf|inf") == 0);
    sv.u = 0xFFF0000000000000UL;                  // -inf
    VD = sv.d;
    snprintf(buf, sizeof buf, "%.2f %E", VD, VD);
    assert(strcmp(buf, "-inf -INF") == 0);
    sv.u = 0x7FF8000000000000UL;                  // quiet nan
    VD = sv.d;
    snprintf(buf, sizeof buf, "%f %G", VD, VD);
    assert(strcmp(buf, "nan NAN") == 0);

    puts("printf f ok");
    return 0;
}
