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

#endif // __UVCLANG_MATH_H__
