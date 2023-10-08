#ifndef __UVM_MATH_H__
#define __UVM_MATH_H__

#include <assert.h>

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

// Convert from degrees to radians
#define DEG2RAD(a) (a * M_PI_F / 180)

#endif
