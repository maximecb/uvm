// Classic recursive Fibonacci, a small call-heavy microbenchmark.

#include <assert.h>
#include <stdio.h>

unsigned long add(unsigned long x, unsigned long y)
{
    return x+y;
}

// Make sure we can compile overloads.
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

int NbFibClassInstances = 0;
class FibClass
{
public:

    FibClass(unsigned long n): _n{n} {
        NbFibClassInstances++;
        printf("FibClass Constructor\n");
    }
    ~FibClass() {
        NbFibClassInstances--;
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
    printf("Before: %d\n", NbFibClassInstances);
    {
        FibClass f(27);
        printf("After Construct: %d\n", NbFibClassInstances);
        assert(NbFibClassInstances == 1);
        unsigned long result = f.exec();
        printf("fib val: %ld\n", result);
    }
    printf("After Destruct: %d\n", NbFibClassInstances);
    return 0;
}
