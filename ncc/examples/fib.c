/*
* Classic recursive fibonacci test / microbenchmark
*/

#include <assert.h>

unsigned long fib(unsigned long n)
{
    if (n < 2)
        return n;

    return fib(n-1) + fib(n-2);
}

void main()
{
    unsigned long r = fib(27);
    assert(r == 196418);
}
