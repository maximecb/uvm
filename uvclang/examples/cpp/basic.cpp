// Classic recursive Fibonacci, a small call-heavy microbenchmark.

#include <assert.h>
#include <stdio.h>

unsigned long add(unsigned long x, unsigned long y)
{
    return x+y;
}

float add(float x, float y)
{
    return x+y;
}

unsigned long fib(unsigned long n)
{
    if (n < 2)
        return n;

    return add(fib(n - 1), fib(n - 2));
}

class FibClass
{
public:
    FibClass(unsigned long n): _n{n} {
        printf("FibClass Constructor\n");
    }
    ~FibClass() {
        printf("FibClass Destructor\n");

    }
    unsigned long exec()
    {
        return fib(_n);
    }

private:
    unsigned long _n;

};


int main(int , char**)
{
    unsigned long r = fib(27);
    printf("fib(27) = %lu\n", r);
    assert(r == 196418);
    {
        FibClass f(27);
        unsigned long result = f.exec();
        printf("class fib(27) = %lu\n", result);
    }
    return 0;
}
