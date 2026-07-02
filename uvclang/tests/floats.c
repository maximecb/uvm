// Exercises uvclang's float (f32) support, differential vs native and
// self-checking. Covers: fadd/fsub/fmul/fdiv, fneg, fcmp (ordered/unordered),
// sitofp/uitofp/fptosi/fptoui, float phi (loop accumulation), float select, and
// the libm f32 calls (sinf/cosf/tanf/sqrtf/fabsf/powf) lowered to UVM ops.
//
// Basic IEEE arithmetic/conversions are asserted *exactly* (identical on the VM
// and native), while the transcendental results are checked with a tolerance —
// UVM computes them via Rust's std while native uses the platform libm, so the
// last bit can differ. Inputs come from a `volatile` seed so -O2 can't const-
// fold main away. The exit code is built only from exact operations, so it is
// identical on both sides for the differential harness to compare.

#include <math.h>
#include <assert.h>

static int approx(float a, float b) { return fabsf(a - b) < 1e-3f; }

int main()
{
    volatile int seed = 5;

    // --- conversions & basic arithmetic (exact) ---
    float a = (float)seed;          // sitofp i32 -> 5.0
    float nb = (float)(seed - 10);  // sitofp of a negative -> -5.0
    unsigned us = (unsigned)seed;
    float au = (float)us;           // uitofp -> 5.0
    assert(a == 5.0f);
    assert(nb == -5.0f);
    assert(au == 5.0f);

    float b = a / 2.0f;             // fdiv -> 2.5
    float c = a * b + 1.0f;         // 5*2.5 + 1 = 13.5 (may be fmuladd)
    float d = -c;                   // fneg -> -13.5
    float e = fabsf(d);             // 13.5
    assert(b == 2.5f);
    assert(c == 13.5f);
    assert(d == -13.5f);
    assert(e == 13.5f);
    assert(c - e == 0.0f);          // fsub

    // --- comparisons (fcmp: ordered and, via !, unordered) ---
    assert(a > b);
    assert(b < a);
    assert(a >= 5.0f && a <= 5.0f);
    assert(d < 0.0f);
    assert(!(a < b));               // unordered/negated path
    assert(a != b);

    // --- float phi via a loop accumulator + uitofp of the index ---
    float acc = 0.0f;
    for (unsigned i = 0; i < 10u; i++)
        acc = acc + (float)i;       // 0+1+...+9 = 45.0 (exact)
    assert(acc == 45.0f);

    // --- float select ---
    float m = (a > b) ? a : nb;     // 5.0
    assert(m == 5.0f);

    // --- transcendentals (tolerant) ---
    assert(approx(sqrtf(a * a), 5.0f));      // sqrt(25) = 5
    assert(approx(sqrtf(2.0f), 1.4142135f));
    assert(approx(sinf(0.0f), 0.0f));
    assert(approx(cosf(0.0f), 1.0f));
    assert(approx(tanf(0.0f), 0.0f));
    assert(approx(sinf(1.0f), 0.8414710f));
    assert(approx(cosf(1.0f), 0.5403023f));
    assert(approx(powf(2.0f, 10.0f), 1024.0f));

    // --- exit code from exact ops only (same on VM and native) ---
    int ci = (int)c;                // fptosi 13.5 -> 13
    unsigned cu = (unsigned)e;      // fptoui 13.5 -> 13
    int ai = (int)acc;              // 45
    int mi = (int)m;                // 5
    // 13 + 13 + 45 + 5 = 76
    return ci + (int)cu + ai + mi;
}
