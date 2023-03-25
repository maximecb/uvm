#include <math.h>
#include <assert.h>

float g = 3.5f;

float f_array[4] = { 1.0f, 2.0f, 3.0f, 4.5f };

int main()
{
    // Int/float casts
    float x = 4.0f;
    assert((int)x == 4);
    assert((int)3.0f == 3);
    assert((int)(float)3 == 3);

    sqrtf(0);
    //assert(sqrtf(0) == 0);

    return 0;
}
