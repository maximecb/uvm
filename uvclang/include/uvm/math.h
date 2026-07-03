#ifndef __UVM_MATH_H__
#define __UVM_MATH_H__

// <uvm/math.h> for the uvclang/UVM target. A handful of small, type-generic
// arithmetic helper macros. They work on integers as-is; LERP/REMAP/DEG2RAD are
// also meaningful with floats. Macro arguments are fully parenthesized to avoid
// precedence surprises.
//
// NOTE: DEG2RAD references M_PI_F, a float constant from <math.h>, which must be
// included for DEG2RAD to be usable.

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
