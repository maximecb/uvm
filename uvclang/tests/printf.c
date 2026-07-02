// Differential test for uvclang's printf (against the native libc printf).
// Exercises the supported conversions: %d %i %u %x %X %o %c %s %% and the
// l-length modifier, plus the return value (chars written). Floating point
// (%f) and %p are intentionally absent: %f needs FP varargs uvclang can't pass
// yet, and %p prints an address that differs between native and UVM.
//
// Note: at -O1/-O2 clang rewrites conversion-free calls like printf("...\n")
// into puts(), on both the native and UVM sides — so this also exercises that
// path. Calls whose return value is used stay as real printf calls.
#include <stdio.h>
#include <limits.h>

int main()
{
    printf("hello world\n");
    printf("ints: %d %i %d\n", 42, -42, 0);
    printf("bounds: %d %d\n", INT_MIN, INT_MAX);
    printf("unsigned: %u %u\n", 0u, 4000000000u);
    printf("hex: %x %X 0x%x\n", 255, 255, 0xdeadbeef);
    printf("octal: %o\n", 64);
    printf("chars: %c%c%c\n", 'a', 'b', 'c');
    printf("strings: [%s] [%s] [%s]\n", "foo", "", "bar baz");
    printf("percent: 100%% done\n");
    printf("mixed: %s = %d (0x%x)\n", "value", 255, 255);

    long big = 1234567890123L;
    printf("longs: %ld %lu %lx\n", big, (unsigned long)big, (unsigned long)big);
    printf("neg long: %ld\n", -big);
    printf("ll: %lld\n", 9000000000000000000LL);

    // Field width, flags and precision (all supported; matched vs native libc).
    printf("width: [%5d][%-5d][%05d][%5d]\n", 42, 42, 42, -42);
    printf("hexw:  [%8x][%08x][%#x][%#08x]\n", 0xabc, 0xabc, 0xabc, 0xabc);
    printf("sign:  [%+d][%+d][% d][%+.5d]\n", 42, -42, 42, 42);
    printf("prec:  [%.3d][%.0d][%.5x][%8.5d][%-8.5d]\n", 7, 0, 0x2a, 42, 42);
    printf("strs:  [%10s][%-10s][%.3s][%8.3s]\n", "hi", "hi", "abcdef", "abcdef");
    printf("octc:  [%#o][%#o][%o][%3c][%-3c]\n", 64, 0, 8, 'x', 'y');
    printf("star:  [%*d][%-*d][%.*s]\n", 6, 42, 6, 42, 3, "abcdef");

    // Return value (number of characters written) must match native.
    int n = printf("measure this line\n");
    printf("wrote %d chars\n", n);

    return 0;
}
