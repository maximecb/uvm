// Differential test for <ctype.h>: uvclang's UVM-side ctype (ASCII classification
// and case conversion) vs the platform's native libc. The UVM build resolves
// <ctype.h> to uvclang/include/ctype.h; the native reference build uses the
// system header. Both must agree on the folded checksum returned as the exit code.
//
// Every character in the well-defined ASCII range [0, 128) is fed through each
// classifier and both case converters; the results are folded into a rolling hash
// so a disagreement on any single character changes the exit code. The loop bounds
// are volatile so -O2 cannot unroll and const-fold the classifiers away -- they
// actually run.
//
// Two portability points:
//   * The classifiers are normalized with `!= 0`: the host's is*() macros return
//     an arbitrary nonzero value for "true", while uvclang's return exactly 1, so
//     the raw ints would differ even when both agree on the classification.
//   * Only bytes in [0, 128) are tested. Passing a negative value (a plain char
//     >= 0x80 sign-extended to int) to the ctype functions is undefined in the C
//     standard, so the two implementations need not agree there.
#include <ctype.h>

int main()
{
    volatile int lo = 0;
    volatile int hi = 128;

    unsigned r = 0;
    for (int c = lo; c < hi; c++)
    {
        r = r * 31u + (isalnum(c)  != 0);
        r = r * 31u + (isalpha(c)  != 0);
        r = r * 31u + (isdigit(c)  != 0);
        r = r * 31u + (islower(c)  != 0);
        r = r * 31u + (isupper(c)  != 0);
        r = r * 31u + (isprint(c)  != 0);
        r = r * 31u + (isspace(c)  != 0);
        r = r * 31u + (iscntrl(c)  != 0);
        r = r * 31u + (isgraph(c)  != 0);
        r = r * 31u + (ispunct(c)  != 0);
        r = r * 31u + (isxdigit(c) != 0);
        r = r * 31u + (isblank(c)  != 0);

        // Case conversion: fold the converted code points themselves (identical
        // across implementations for c in [0, 128)).
        r = r * 31u + (unsigned)tolower(c);
        r = r * 31u + (unsigned)toupper(c);
    }

    return (int)(r & 0xFF);
}
