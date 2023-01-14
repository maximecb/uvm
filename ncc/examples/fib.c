/*
Classic recursive fibonacci test / microbenchmark
*/

u64 fib(u64 n)
{
    if (n < 2)
        return n;

    return fib(n-1) + fib(n-2);
}

void main()
{
    fib(27);
}
