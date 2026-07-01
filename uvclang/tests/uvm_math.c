// Self-checking test for <uvm/math.h> (the MIN/MAX/CLAMP/REMAP/LERP macros).
// Runs under run_uvm_tests.sh: exits 0 iff every check passes. A volatile seed
// keeps -O2 from const-folding the whole computation away.
#include <assert.h>
#include <uvm/math.h>

int main()
{
    volatile int s = 3;
    int a = s;                          // 3
    int b = s + 4;                      // 7

    int r = 0;
    r += MIN(a, b);                     // 3
    r += MAX(a, b);                     // 7
    r += CLAMP(a, 4, 6);                // 4
    r += CLAMP(b, 1, 5);                // 5
    r += REMAP(s + 2, 0, 10, 0, 100);   // REMAP(5, [0,10]->[0,100]) = 50
    r += LERP(10, 20, 1);               // (1-1)*10 + 1*20 = 20

    assert(r == 89);
    return 0;
}
