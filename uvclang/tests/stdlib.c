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

    return r;                       // exit 76
}
