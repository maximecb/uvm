// goto-driven control flow (a hand-rolled loop via labels) and deeper
// recursion than recursion.c.

int collatz(int n)
{
    int steps = 0;
loop:
    if (n == 1) goto done;
    if (n & 1) n = 3 * n + 1;
    else       n = n / 2;
    steps++;
    goto loop;
done:
    return steps;
}

int sum_rec(int n) { return n == 0 ? 0 : n + sum_rec(n - 1); } // recursion depth n

int main()
{
    return collatz(27)     // 111 steps
         + sum_rec(100);   // 5050
}
