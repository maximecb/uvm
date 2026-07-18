// Exercises the <math.h> functions that are NOT a single UVM instruction:
// floor/ceil (expanded inline from the @llvm.floor/@llvm.ceil intrinsics) and
// atan2/fmod (portable C in the header, on top of atan and the float ops).
//
// Exact cases (floor/ceil on representable values, fmod's true remainder) are
// asserted with `==`, so they must be bit-identical on the VM and native.
// atan2 rides on the tolerant `atan` instruction, so it is checked with a
// tolerance. The exit code is built only from exact ops, keeping it identical
// on both sides for the differential harness.

#include <math.h>
#include <assert.h>

static int approx(double a, double b)  { return fabs(a - b)  < 1e-6;  }
static int approxf(float a, float b)    { return fabsf(a - b) < 1e-4f; }

int main()
{
    // --- floor/ceil: sign, fraction, and already-integral inputs ---
    assert(floor(2.5) == 2.0   && ceil(2.5) == 3.0);
    assert(floor(-2.5) == -3.0 && ceil(-2.5) == -2.0);
    assert(floor(3.0) == 3.0   && ceil(3.0) == 3.0);
    assert(floor(-0.4) == -1.0 && ceil(-0.4) == 0.0);
    assert(floorf(2.5f) == 2.0f && ceilf(-2.5f) == -2.0f);

    // Magnitudes at/above 2^52 (f64) / 2^23 (f32) are already integral and must
    // pass through the guard unchanged.
    assert(floor(1e16) == 1e16 && ceil(-1e16) == -1e16);
    assert(floor(4503599627370496.0) == 4503599627370496.0);
    assert(floorf(1.0e8f) == 1.0e8f && ceilf(-1.0e8f) == -1.0e8f);

    // Non-integral just below the integral threshold still rounds.
    assert(floor(1099511627776.5) == 1099511627776.0);   // 2^40 + 0.5
    assert(ceil(1099511627776.5)  == 1099511627777.0);

    // --- fmod: sign of x, exact remainders, huge ratio, |x| < |y| ---
    assert(fmod(10.0, 3.0) == 1.0);
    assert(fmod(-10.0, 3.0) == -1.0);
    assert(fmod(10.0, -3.0) == 1.0);
    assert(fmod(7.0, 7.0) == 0.0);
    assert(fmod(1.0, 3.0) == 1.0);              // |x| < |y| -> x
    assert(fmod(1e300, 7.0) == 1.0);            // exact across a 2^996 ratio
    assert(fmod(5.3, 2.0) == 5.3 - 4.0);        // true remainder, bit-exact
    assert(fmodf(10.0f, 3.0f) == 1.0f);
    assert(fmodf(-5.5f, 2.0f) == -1.5f);

    // --- atan2: one point per quadrant plus the axes (tolerant) ---
    assert(approx(atan2(1.0, 1.0),   M_PI_4));
    assert(approx(atan2(1.0, -1.0),  3.0 * M_PI_4));
    assert(approx(atan2(-1.0, 1.0),  -M_PI_4));
    assert(approx(atan2(-1.0, -1.0), -3.0 * M_PI_4));
    assert(approx(atan2(1.0, 0.0),   M_PI_2));
    assert(approx(atan2(-1.0, 0.0),  -M_PI_2));
    assert(approx(atan2(0.0, 1.0),   0.0));
    assert(approx(atan2(0.0, -1.0),  M_PI));
    assert(approxf(atan2f(3.0f, 4.0f), 0.6435011f));

    // --- exit code from exact ops only (same on VM and native) ---
    int a = (int)floor(100.7);   // 100
    int b = (int)ceil(100.7);    // 101
    int c = (int)fmod(100.0, 7.0); // 2
    return a + b + c;            // 203
}
