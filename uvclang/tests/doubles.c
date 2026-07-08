// Exercises uvclang's double (f64) support, differential vs native and
// self-checking. Covers: fadd/fsub/fmul/fdiv, fneg, fcmp (ordered/unordered),
// sitofp/uitofp/fptosi/fptoui, double phi (loop accumulation), double select,
// the libm f64 calls (sin/cos/tan/sqrt/fabs/pow) lowered to UVM ops, and the
// float<->double conversions (fpext/fptrunc).
//
// Basic IEEE arithmetic/conversions are asserted *exactly* (identical on the VM
// and native), while the transcendental results are checked with a tolerance —
// UVM computes them via Rust's std while native uses the platform libm, so the
// last bit can differ. Inputs come from a `volatile` seed so -O2 can't const-
// fold main away. The exit code is built only from exact operations, so it is
// identical on both sides for the differential harness to compare.

#include <math.h>
#include <assert.h>

static int approx(double a, double b) { return fabs(a - b) < 1e-9; }

int main()
{
    volatile int seed = 5;

    // --- conversions & basic arithmetic (exact) ---
    double a = (double)seed;         // sitofp i32 -> 5.0
    double nb = (double)(seed - 10); // sitofp of a negative -> -5.0
    unsigned us = (unsigned)seed;
    double au = (double)us;          // uitofp -> 5.0
    assert(a == 5.0);
    assert(nb == -5.0);
    assert(au == 5.0);

    double b = a / 2.0;              // fdiv -> 2.5
    double c = a * b + 1.0;          // 5*2.5 + 1 = 13.5 (may be fmuladd)
    double d = -c;                   // fneg -> -13.5
    double e = fabs(d);              // 13.5
    assert(b == 2.5);
    assert(c == 13.5);
    assert(d == -13.5);
    assert(e == 13.5);
    assert(c - e == 0.0);           // fsub

    // --- comparisons (fcmp: ordered and, via !, unordered) ---
    assert(a > b);
    assert(b < a);
    assert(a >= 5.0 && a <= 5.0);
    assert(d < 0.0);
    assert(!(a < b));               // unordered/negated path
    assert(a != b);

    // --- double phi via a loop accumulator + uitofp of the index ---
    double acc = 0.0;
    for (unsigned i = 0; i < 10u; i++)
        acc = acc + (double)i;      // 0+1+...+9 = 45.0 (exact)
    assert(acc == 45.0);

    // --- double select ---
    double m = (a > b) ? a : nb;    // 5.0
    assert(m == 5.0);

    // --- float <-> double conversions (fpext / fptrunc), exact here ---
    float f = (float)c;             // fptrunc 13.5 -> 13.5f (exact)
    double back = (double)f;        // fpext 13.5f -> 13.5 (exact)
    assert(f == 13.5f);
    assert(back == 13.5);

    // --- genuine f64 precision (would fail if we truncated to f32) ---
    double big = (double)seed + 16777212.0; // 2^24 + 1 = 16777217, exact in f64
    assert(big == 16777217.0);               // f32 would round this to 16777216
    assert((double)(float)big == 16777216.0);// narrowing to f32 loses the +1

    // --- transcendentals (tolerant) ---
    assert(approx(sqrt(a * a), 5.0));
    assert(approx(sqrt(2.0), 1.4142135623730951));
    assert(approx(sin(0.0), 0.0));
    assert(approx(cos(0.0), 1.0));
    assert(approx(tan(0.0), 0.0));
    assert(approx(sin(1.0), 0.8414709848078965));
    assert(approx(cos(1.0), 0.5403023058681398));
    assert(approx(pow(2.0, 10.0), 1024.0));

    // --- exit code from exact ops only (same on VM and native) ---
    int ci = (int)c;                // fptosi 13.5 -> 13
    unsigned cu = (unsigned)e;      // fptoui 13.5 -> 13
    int ai = (int)acc;              // 45
    int mi = (int)m;                // 5
    // 13 + 13 + 45 + 5 = 76
    return ci + (int)cu + ai + mi;
}
