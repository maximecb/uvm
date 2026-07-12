// Runtime-sized stack allocation: alloca() (from <alloca.h>) and C99
// variable-length arrays. Both lower to a dynamic LLVM `alloca`; VLAs also emit
// llvm.stacksave/llvm.stackrestore around their scope. Differential vs native.

#include <stdio.h>
#include <string.h>
#include <alloca.h>

// alloca() with a runtime size: fill the buffer and checksum it.
static int fill_sum(int n)
{
    unsigned char *buf = (unsigned char *)alloca(n);
    for (int i = 0; i < n; i++)
        buf[i] = (unsigned char)(i * 7 + 3);
    int s = 0;
    for (int i = 0; i < n; i++)
        s += buf[i];
    return s;
}

// Two independent allocas live at once in the same frame.
static int two_allocas(int n)
{
    int *a = (int *)alloca(n * sizeof(int));
    int *b = (int *)alloca(n * sizeof(int));
    for (int i = 0; i < n; i++) { a[i] = i; b[i] = 2 * i; }
    int s = 0;
    for (int i = 0; i < n; i++)
        s += a[i] + b[i];
    return s;
}

// C99 VLA in a nested scope: clang brackets it with llvm.stacksave/
// llvm.stackrestore, so each iteration's array is reclaimed before the next.
static int vla_sum(int n)
{
    int total = 0;
    for (int r = 0; r < 4; r++)
    {
        long row[n];
        for (int i = 0; i < n; i++)
            row[i] = (long)i * (r + 1);
        for (int i = 0; i < n; i++)
            total += (int)row[i];
    }
    return total;
}

// Expected byte at index i for a given seed (shared by producer and checker).
static unsigned char pattern(int i, int seed)
{
    return (unsigned char)(((seed + i) * 31 + 7) & 0xFF);
}

// Callee that fills the caller's buffer. It first does its own stack work (a
// VLA), so its frame sits above the caller's alloca region -- if alloca'd
// memory did not outlive the call, or the callee's frame overlapped it, the
// buffer would come back corrupted.
static void produce(unsigned char *buf, int n, int seed)
{
    long scratch[n];
    for (int i = 0; i < n; i++)
        scratch[i] = (long)pattern(i, seed);
    for (int i = 0; i < n; i++)
        buf[i] = (unsigned char)scratch[i];
}

// Allocate a runtime-sized buffer, poison it, have a callee fill it, then
// validate the contents after the call returns. Returns the number of bytes
// that came back wrong (0 == the buffer survived the call intact).
static int alloca_across_call(int n, int seed)
{
    unsigned char *buf = (unsigned char *)alloca(n);
    for (int i = 0; i < n; i++)
        buf[i] = 0xEE;                  // poison: the callee must overwrite it
    produce(buf, n, seed);
    int bad = 0;
    for (int i = 0; i < n; i++)
        if (buf[i] != pattern(i, seed))
            bad++;
    return bad;
}

// Reclaim check: a large alloca in a function called many times. If the frame
// were not reset on return, the software stack would overflow long before the
// loop ends; a correct epilogue keeps peak usage at one buffer.
static int churn(int k)
{
    char *p = (char *)alloca(4096);
    memset(p, k & 0x7F, 4096);
    return (unsigned char)p[k % 4096];
}

int main(void)
{
    int a = fill_sum(41);
    int b = two_allocas(16);
    int c = vla_sum(9);
    int bad = alloca_across_call(50, 100);   // 0 if the buffer survived the call

    long acc = 0;
    for (int i = 0; i < 5000; i++)
        acc += churn(i);

    printf("a=%d b=%d c=%d bad=%d acc=%ld\n", a, b, c, bad, acc);
    return (a + b + c + bad) & 0x7F;
}
