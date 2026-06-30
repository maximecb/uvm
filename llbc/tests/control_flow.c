// Exercises function bodies: arithmetic, comparisons, branches, loops
// (phi nodes at -O2), switch, and calls.

int fib(int n)
{
    int a = 0, b = 1;
    for (int i = 0; i < n; i++) {
        int t = a + b;
        a = b;
        b = t;
    }
    return a;
}

int classify(int x)
{
    switch (x) {
        case 0:  return 100;
        case 1:  return 200;
        case 2:  return 300;
        default: return -1;
    }
}

int sum_to(int n)
{
    int s = 0;
    for (int i = 1; i <= n; i++)
        s += i;
    return s;
}

int main()
{
    return fib(10) + classify(2) + sum_to(5);
}
