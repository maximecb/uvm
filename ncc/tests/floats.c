#include <math.h>
#include <assert.h>

float g = 3.5f;

float f_array[4] = { 1.0f, 2.0f, 3.0f, 4.5f };

float fun(float f)
{
    return f;
}

int main()
{
    // Int/float casts
    float x = 4.0f;
    float y = 5;
    assert((int)x == 4);
    assert((int)3.0f == 3);
    assert((int)(float)3 == 3);
    assert((int)-5000.0f == -5000);
    assert((short)5000.0f == 5000);
    assert((short)-5000.0f == (short)-5000);
    assert((float)(short)5000 == 5000.0f);

    // Global variable access
    assert(g == 3.5f);
    g = 4.0f;
    assert(g == 4.0f);
    assert(f_array[0] == 1.0f);

    // Automatic promotion of int argument to float
    assert(fun(4) == 4.0f);

    // Floating-point comparisons
    assert(0.0f == 0.0f);
    assert(0.0f != 1.0f);
    assert(0.0f < 1.0f);
    assert(-1.0f < 0.0f);

    // Arithmetic
    assert(1.0f + 2.0f == 3.0f);
    assert(2.0f * 3.0f == 6.0f);
    assert(6.0f / 2.0f == 3.0f);

    assert(powf(0.0f, 1.0f) == 0.0f);
    assert(powf(1.0f, 2.0f) == 1.0f);
    assert(powf(2.0f, 3.0f) == 8.0f);
    assert(sqrtf(0.0f) == 0.0f);
    assert(sqrtf(4.0f) == 2.0f);
    assert(sinf(0.0f) == 0.0f);

    assert(floorf(4.5f) == 4.0f);
    assert(floorf(-4.5f) == -5.0f);

    return 0;
}
