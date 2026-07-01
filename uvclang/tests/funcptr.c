// Function pointers and indirect calls (call_fp), incl. an array of pointers.

int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }
int mul(int a, int b) { return a * b; }

int apply(int (*op)(int, int), int a, int b) { return op(a, b); }

int main()
{
    int (*ops[3])(int, int) = { add, sub, mul };
    int total = 0;
    for (int i = 0; i < 3; i++)
        total += apply(ops[i], 10, 3);
    return total;   // 13 + 7 + 30 = 50
}
