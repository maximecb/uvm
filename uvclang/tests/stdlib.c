// Differential test for <stdlib.h>: uvclang's UVM-side stdlib vs the platform's
// native libc. The UVM build resolves <stdlib.h> to uvclang/include/stdlib.h;
// the native reference build uses the system header. Both must agree on the
// computed exit code.
//
// Exercises abs() and a malloc/write/read/free round-trip -- pointer values
// themselves are never observed, only the values read back, so the bump
// allocator and the host malloc agree. rand()/srand() and itoa()/ltoa() are
// ported too but are not diff-tested here (rand's sequence differs from the
// host's; itoa/ltoa are not in the host libc). A volatile seed blocks -O2 from
// const-folding the whole computation.
#include <stdlib.h>
#include <limits.h>   // LONG_MAX / LONG_MIN (strtol overflow saturation)

int main()
{
    volatile int seed = 6;
    int r = 0;

    r += abs(seed - 20);            // abs(-14) = 14
    r += abs(seed);                 // 6

    // malloc / free round-trip: fill a buffer, sum it, free it.
    int n = seed;                   // 6
    int *buf = (int *)malloc((size_t)n * sizeof(int));
    for (int i = 0; i < n; i++)
        buf[i] = i * i;
    long sum = 0;
    for (int i = 0; i < n; i++)
        sum += buf[i];              // 0+1+4+9+16+25 = 55
    free(buf);

    // A second allocation must not overlap the first (freed) one's live use.
    char *s = (char *)malloc(4);
    s[0] = 'o'; s[1] = 'k'; s[2] = '\0';
    r += (s[0] == 'o' && s[1] == 'k' && s[2] == '\0');  // 1
    free(s);

    r += (int)sum;                  // 14 + 6 + 1 + 55 = 76

    // labs.
    r += (labs(seed - 20) == 14);   // 1
    r += (labs((long)seed) == 6);   // 1

    // atoi: plain, leading spaces + sign, trailing garbage, no digits.
    r += (atoi("42") == 42);        // 1
    r += (atoi("  -13") == -13);    // 1
    r += (atoi("7abc") == 7);       // 1
    r += (atoi("nope") == 0);       // 1

    // strtol: base 10, endptr, hex (explicit + auto), octal (auto), sign,
    // stop-at-garbage endptr, base 36, and the empty (no-conversion) case.
    char *end;
    r += (strtol("100", &end, 10) == 100);   // 1
    r += (*end == '\0');                      // 1
    r += (strtol("0xFF", NULL, 16) == 255);  // 1
    r += (strtol("0x1A", NULL, 0) == 26);    // 1
    r += (strtol("077", NULL, 0) == 63);     // 1
    r += (strtol("+21", NULL, 10) == 21);    // 1
    long v = strtol("  -21rest", &end, 10);  // -21, end at 'r'
    r += (v == -21);                          // 1
    r += (*end == 'r');                       // 1
    r += (strtol("z", NULL, 36) == 35);      // 1
    r += (strtol("", &end, 10) == 0);        // 1 (no digits converted)
    r += (strtol("99999999999999999999", NULL, 10) == LONG_MAX);   // 1 (clamps)
    r += (strtol("-99999999999999999999", NULL, 10) == LONG_MIN);  // 1 (clamps)

    // 76 + 2 (labs) + 4 (atoi) + 12 (strtol) = 94
    return r;                       // exit 94
}
