#ifndef __UVM_MATH_H__
#define __UVM_MATH_H__

//#include <assert.h>

// Minimum of two values
#ifndef MIN
#define MIN(a, b) (a < b? a:b)
#endif

// Maximum of two values
#ifndef MAX
#define MAX(a, b) (a > b? a:b)
#endif

// Clamp a value between min and max
#ifndef CLAMP
#define CLAMP(x, min, max) MIN(MAX(x, min), max)
#endif

// Remap a value from range [a0, a1] into range [b0, b1]
#ifndef REMAP
#define REMAP(v, a0, a1, b0, b1) (b0 + (b1 - b0) * ((v) - a0) / (a1 - a0))
#endif

// Convert from degrees to radians
#ifndef DEG2RAD
#define DEG2RAD(a) (a * M_PI_F / 180)
#endif

#endif
