#ifndef __MATH_H__
#define __MATH_H__

// Float constants
#define M_PI_F 3.14159266f

// Here we use macros for performance because
// ncc doesn't yet support inline functions
#define sinf(__f) (asm (__f) -> float { sin_f32; })
#define cosf(__f) (asm (__f) -> float { cos_f32; })
#define tanf(__f) (asm (__f) -> float { tan_f32; })
#define atanf(__f) (asm (__f) -> float { atans_f32; })
#define sqrtf(__f) (asm (__f) -> float { sqrt_f32; })

float floorf(float x)
{
    float xi = (float)(int)x;

    if (x < xi)
        return xi - 1.0f;

    return xi;
}

#endif
