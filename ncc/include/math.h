#ifndef __MATH_H__
#define __MATH_H__

// Constants for pi, pi/2, pi/4
#define M_PI 3.14159265358979323846264338328
#define M_PI_2 1.57079632679489661923132169164
#define M_PI_4 0.78539816339744830961566084582

// Here we use macros for performance because
// ncc doesn't yet support inline functions
#define sinf(__f) (asm (__f) -> float { sin_f32; })
#define cosf(__f) (asm (__f) -> float { cos_f32; })
#define atanf(__f) (asm (__f) -> float { atans_f32; })
#define sqrtf(__f) (asm (__f) -> float { sqrt_f32; })

#endif
