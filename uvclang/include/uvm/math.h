#ifndef __UVM_MATH_H__
#define __UVM_MATH_H__

// <uvm/math.h> for the uvclang/UVM target. Ported from ncc/include/uvm/math.h.
// A handful of small, type-generic arithmetic helper macros used by ncc's UVM
// programs. They work on integers as-is; LERP/REMAP/DEG2RAD are meaningful with
// floats once uvclang gains float support.
//
// Fixes vs the ncc original: that file's LERP block is missing its #endif and
// carries a stray `#define LERP`, which a strict (clang) preprocessor rejects as
// an unterminated conditional; macro arguments are also fully parenthesized here
// to avoid precedence surprises.
//
// NOTE: DEG2RAD references M_PI_F, a float constant that lives in <math.h>.
// uvclang has no <math.h> yet (no float support), so DEG2RAD is only usable once
// that lands; it is defined here for source compatibility.

// Minimum of two values
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

// Maximum of two values
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

// Clamp a value between min and max
#ifndef CLAMP
#define CLAMP(x, min, max) MIN(MAX((x), (min)), (max))
#endif

// Linear interpolation between two values
#ifndef LERP
#define LERP(a, b, x) ((1 - (x)) * (a) + ((x) * (b)))
#endif

// Remap a value from range [a0, a1] into range [b0, b1]
#ifndef REMAP
#define REMAP(v, a0, a1, b0, b1) ((b0) + ((b1) - (b0)) * ((v) - (a0)) / ((a1) - (a0)))
#endif

// Convert from degrees to radians
#ifndef DEG2RAD
#define DEG2RAD(a) ((a) * M_PI_F / 180)
#endif

#endif // __UVM_MATH_H__
