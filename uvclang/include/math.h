#ifndef __UVCLANG_MATH_H__
#define __UVCLANG_MATH_H__

// Minimal ISO <math.h> for the uvclang/UVM target.
//
// UVM has both single-precision (f32) and double-precision (f64) floating-point
// ops, so this header declares the `float` (`*f`) functions and their `double`
// counterparts. uvclang lowers each inline to the matching UVM `*_f32`/`*_f64`
// instruction (see codegen's `is_float_builtin` / `gen_intrinsic`).
//
// Constants are provided in both the traditional `double` spelling and a `*_F`
// `float` spelling (referenced by <uvm/math.h>'s DEG2RAD).

#define M_E_F        2.71828182845904523536f
#define M_LOG2E_F    1.44269504088896340736f
#define M_LOG10E_F   0.43429448190325182765f
#define M_LN2_F      0.69314718055994530942f
#define M_LN10_F     2.30258509299404568402f
#define M_PI_F       3.14159265358979323846f
#define M_PI_2_F     1.57079632679489661923f
#define M_PI_4_F     0.78539816339744830962f
#define M_SQRT2_F    1.41421356237309504880f
#define M_SQRT1_2_F  0.70710678118654752440f

#ifndef M_PI
#define M_E          2.71828182845904523536
#define M_LOG2E      1.44269504088896340736
#define M_LOG10E     0.43429448190325182765
#define M_LN2        0.69314718055994530942
#define M_LN10       2.30258509299404568402
#define M_PI         3.14159265358979323846
#define M_PI_2       1.57079632679489661923
#define M_PI_4       0.78539816339744830962
#define M_SQRT2      1.41421356237309504880
#define M_SQRT1_2    0.70710678118654752440
#endif

// Single-precision functions backed by UVM f32 instructions.
float sinf(float x);
float cosf(float x);
float tanf(float x);
float asinf(float x);
float acosf(float x);
float atanf(float x);
float sqrtf(float x);
float fabsf(float x);
float powf(float x, float y);
float exp2f(float x);

// Double-precision functions backed by UVM f64 instructions.
double sin(double x);
double cos(double x);
double tan(double x);
double asin(double x);
double acos(double x);
double atan(double x);
double sqrt(double x);
double fabs(double x);
double pow(double x, double y);
double exp2(double x);

// These have no UVM instruction, but clang (even at -O0) lowers every call to an
// `@llvm.*` intrinsic — `floor`/`ceil`/`trunc`/`round`, `minnum`/`maxnum` for
// fmin/fmax, and `copysign` — which uvclang expands inline (see codegen's
// `gen_floor_ceil`/`gen_trunc`/`gen_round`/`gen_fminmax`/`gen_copysign`). These
// declarations only give the caller a prototype — the intrinsic never reaches a
// real function body.
float  floorf(float x);
float  ceilf(float x);
double floor(double x);
double ceil(double x);
float  truncf(float x);
double trunc(double x);
float  roundf(float x);
double round(double x);
float  fminf(float x, float y);
float  fmaxf(float x, float y);
double fmin(double x, double y);
double fmax(double x, double y);
float  copysignf(float x, float y);
double copysign(double x, double y);

// atan2 and fmod stay ordinary libm calls in clang's output (no intrinsic), so
// they are implemented here in portable C on top of the UVM-backed primitives
// above. `static inline` keeps each translation unit self-contained (no libm to
// link) without risking duplicate definitions.

// atan2(y, x): quadrant-correct arctangent, built from the single-argument
// `atan`. `atan(y/x)` gives the right angle in (-pi/2, pi/2) for x > 0; the
// half-plane x < 0 is pi away, and the x == 0 axis is +-pi/2 by sign of y.
static inline float atan2f(float y, float x)
{
    if (x > 0.0f) return atanf(y / x);
    if (x < 0.0f) return atanf(y / x) + (y >= 0.0f ? M_PI_F : -M_PI_F);
    return y > 0.0f ? M_PI_2_F : (y < 0.0f ? -M_PI_2_F : 0.0f);
}

static inline double atan2(double y, double x)
{
    if (x > 0.0) return atan(y / x);
    if (x < 0.0) return atan(y / x) + (y >= 0.0 ? M_PI : -M_PI);
    return y > 0.0 ? M_PI_2 : (y < 0.0 ? -M_PI_2 : 0.0);
}

// fmod(x, y): the remainder x - n*y (n truncated toward zero), with the sign of
// x. Computed by binary long division entirely in the float domain so it stays
// exact for any ratio: scale |y| up to the largest |y|*2^k <= |x|, then walk the
// scale back down subtracting where it fits. Each subtract keeps |x| < 2*scale,
// so it is exact (Sterbenz), and doubling/halving the scale is exact too.
// Undefined inputs (y == 0, non-finite x, NaN y) return NaN, matching C.
static inline float fmodf(float x, float y)
{
    float a = fabsf(x), b = fabsf(y);
    if (a != a || a == __builtin_inff() || y == 0.0f || b != b)
        return __builtin_nanf("");
    if (a < b) return x;               // |x| < |y|: remainder is x itself
    float t = b;
    while (t + t <= a) t += t;          // t = largest |y|*2^k with t <= |x|
    while (t >= b) {
        if (a >= t) a -= t;
        t *= 0.5f;
    }
    return x < 0.0f ? -a : a;
}

static inline double fmod(double x, double y)
{
    double a = fabs(x), b = fabs(y);
    if (a != a || a == __builtin_inf() || y == 0.0 || b != b)
        return __builtin_nan("");
    if (a < b) return x;               // |x| < |y|: remainder is x itself
    double t = b;
    while (t + t <= a) t += t;          // t = largest |y|*2^k with t <= |x|
    while (t >= b) {
        if (a >= t) a -= t;
        t *= 0.5;
    }
    return x < 0.0 ? -a : a;
}

// hypot/cbrt/lround stay ordinary libm calls in clang's output (no intrinsic),
// so they are implemented here in portable C on top of the primitives above.

// hypot(x, y) = sqrt(x*x + y*y), factored through the larger magnitude so the
// intermediate square can't overflow while the true result is still finite.
static inline float hypotf(float x, float y)
{
    x = fabsf(x); y = fabsf(y);
    float hi = x > y ? x : y, lo = x > y ? y : x;
    if (hi == 0.0f) return 0.0f;
    float r = lo / hi;
    return hi * sqrtf(1.0f + r * r);
}

static inline double hypot(double x, double y)
{
    x = fabs(x); y = fabs(y);
    double hi = x > y ? x : y, lo = x > y ? y : x;
    if (hi == 0.0) return 0.0;
    double r = lo / hi;
    return hi * sqrt(1.0 + r * r);
}

// cbrt(x) = sign(x) * |x|^(1/3), on the UVM-backed pow.
static inline float  cbrtf(float x) { return copysignf(powf(fabsf(x), 1.0f / 3.0f), x); }
static inline double cbrt(double x) { return copysign(pow(fabs(x), 1.0 / 3.0), x); }

// lround(x): round to nearest (ties away from zero), returned as an integer.
static inline long lroundf(float x) { return (long)roundf(x); }
static inline long lround(double x) { return (long)round(x); }

#endif // __UVCLANG_MATH_H__
