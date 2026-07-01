// while, do-while, nested loops, break and continue.

int sum_while(int n)
{
    int s = 0, i = 1;
    while (i <= n) { s += i; i++; }
    return s;
}

int sum_dowhile(int n)
{
    int s = 0, i = 1;
    do { s += i; i++; } while (i <= n);
    return s;
}

int count_pairs(int n)
{
    int c = 0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) continue;
            if (i + j > n) break;
            c++;
        }
    }
    return c;
}

int main()
{
    return sum_while(10)      // 55
         + sum_dowhile(5)     // 15
         + count_pairs(6);
}
