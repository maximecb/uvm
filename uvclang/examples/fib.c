// Classic recursive Fibonacci, a small call-heavy microbenchmark.

#include <assert.h>
#include <stdio.h>

unsigned long fib(unsigned long n)
{
    if (n < 2)
        return n;

    return fib(n - 1) + fib(n - 2);
}

int main(void)
{
    unsigned long r = fib(27);
    printf("fib(27) = %lu\n", r);
    assert(r == 196418);
    return 0;
}
