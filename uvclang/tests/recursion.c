// Recursion, mutual recursion, and nested recursive calls.

int fact(int n) { return n <= 1 ? 1 : n * fact(n - 1); }

int is_even(int n);
int is_odd(int n)  { return n == 0 ? 0 : is_even(n - 1); }
int is_even(int n) { return n == 0 ? 1 : is_odd(n - 1); }

int ack(int m, int n)
{
    if (m == 0) return n + 1;
    if (n == 0) return ack(m - 1, 1);
    return ack(m - 1, ack(m, n - 1));
}

int main()
{
    return fact(5) % 100   // 20
         + is_even(10)     // 1
         + is_odd(7)       // 1
         + ack(2, 3);      // 9
}
