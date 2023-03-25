#include <math.h>
#include <assert.h>

int main()
{
    // Int/float casts
    assert((int)3.0f == 3);
    assert((int)(float)3 == 3);

    sqrtf(0);
    //assert(sqrtf(0) == 0);

    return 0;
}
