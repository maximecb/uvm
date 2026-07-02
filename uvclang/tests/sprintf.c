// Differential test for uvclang's sprintf / snprintf against native libc.
// Exercises formatting into a buffer, the C99 snprintf return value (the length
// that *would* have been written), truncation to size-1 + NUL, and the size==0
// case. The formatted buffers are printed so the harness compares them on
// stdout; no floating point (unsupported in varargs).
#include <stdio.h>
#include <string.h>

int main()
{
    char buf[64];
    int n;

    n = sprintf(buf, "hello %d %s!", 42, "world");
    printf("[%s] n=%d\n", buf, n);

    n = sprintf(buf, "%08x/%+d/%.3s/%5d", 0xabc, 7, "abcdef", 9);
    printf("[%s] n=%d\n", buf, n);

    // snprintf truncation: 12-char content into an 8-byte buffer.
    char small[8];
    n = snprintf(small, sizeof small, "abcdefghijkl");
    printf("[%s] n=%d len=%d\n", small, n, (int)strlen(small));

    // snprintf that exactly fills the buffer ("1234567" = 7 chars + NUL).
    n = snprintf(small, sizeof small, "%d", 1234567);
    printf("[%s] n=%d len=%d\n", small, n, (int)strlen(small));

    // size == 0: nothing written, but the would-be length is returned.
    n = snprintf(small, 0, "xyz%d", 99);
    printf("n0=%d\n", n);

    // Build a string incrementally with snprintf return values.
    char line[32];
    int off = 0;
    off += snprintf(line + off, sizeof line - off, "a=%d", 1);
    off += snprintf(line + off, sizeof line - off, ",b=%d", 2);
    off += snprintf(line + off, sizeof line - off, ",c=%s", "z");
    printf("[%s] off=%d\n", line, off);

    return 0;
}
