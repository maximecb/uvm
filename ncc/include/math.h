#ifndef __MATH_H__
#define __MATH_H__

// Float constants
#define M_PI_F 3.14159266f

// Here we use macros for performance because
// ncc doesn't yet support inline functions
#define sinf(f) (asm (f) -> float { sin_f32; })
#define cosf(f) (asm (f) -> float { cos_f32; })
#define tanf(f) (asm (f) -> float { tan_f32; })
#define atanf(f) (asm (f) -> float { atans_f32; })
#define powf(x, y) (asm (x, y) -> float { pow_f32; })
#define sqrtf(f) (asm (f) -> float { sqrt_f32; })

float fabsf(float x)
{
    if (x < 0)
        return -x;
    return x;
}

float floorf(float x)
{
    float xi = (float)(int)x;

    if (x < xi)
        return xi - 1.0f;

    return xi;
}

#endif
