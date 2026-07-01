// Pointer arithmetic, one-past-the-end pointers, pointer differences (which
// divide by the element stride), and element types of different sizes.

static int  ints[5]  = { 10, 20, 30, 40, 50 };
static char bytes[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };

int sum_range(const int *begin, const int *end)   // half-open [begin, end)
{
    int s = 0;
    for (const int *p = begin; p != end; p++)
        s += *p;
    return s;
}

long index_of(const int *base, const int *p) { return p - base; }  // ptr diff / stride

int byte_sum(const char *b, int n)                // 1-byte element stride
{
    int s = 0;
    for (int i = 0; i < n; i++)
        s += b[i];
    return s;
}

int main()
{
    int  s   = sum_range(ints, ints + 5);   // 150 (one-past-end sentinel)
    long idx = index_of(ints, &ints[3]);    // 3
    int  bs  = byte_sum(bytes, 8);          // 36
    return s + (int)idx + bs;               // 189
}
