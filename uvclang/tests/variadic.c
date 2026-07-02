// Callee-side variadic functions via <stdarg.h> (a freestanding clang header):
// va_start / va_arg / va_end. clang expands each va_arg into an x86_64 SysV
// va_list walk (a %struct.__va_list_tag with a register-save area + overflow
// area); uvclang materializes those areas in the prologue of a variadic function
// (see gen_va_start in codegen.rs). This is a differential test of that emulation
// against native libc.
//
// Coverage aimed at the parts most likely to break:
//   * more than 6 total arguments, so the ABI walk spills out of the register
//     save area (args 1..6) into the overflow area (args 7..) -- both va_arg
//     paths, and uvclang's B[6] boundary between them;
//   * argument widths promoted into varargs: int (32-bit slots, load_u32) and
//     long / pointer (64-bit slots, load_u64).
// Floating-point varargs are intentionally omitted (unsupported: they use the
// XMM save area indexed by fp_offset).
#include <stdarg.h>

// Sum `count` int arguments. With count > 6 this spills into the overflow area.
long sum_ints(int count, ...)
{
    va_list ap;
    va_start(ap, count);
    long total = 0;
    for (int i = 0; i < count; i++)
        total += va_arg(ap, int);
    va_end(ap);
    return total;
}

// Sum `count` long (64-bit) arguments.
long sum_longs(int count, ...)
{
    va_list ap;
    va_start(ap, count);
    long total = 0;
    for (int i = 0; i < count; i++)
        total += va_arg(ap, long);
    va_end(ap);
    return total;
}

// Walk a NULL-terminated list of C-string pointers, summing their first bytes.
// Exercises pointer-typed va_arg.
int sum_first_bytes(const char *first, ...)
{
    va_list ap;
    va_start(ap, first);
    int total = 0;
    for (const char *s = first; s != 0; s = va_arg(ap, const char *))
        total += (unsigned char)s[0];
    va_end(ap);
    return total;
}

int main()
{
    long r = 0;

    // 9 int args -> 6 via the register save area + 3 via the overflow area.
    r += sum_ints(9, 1, 2, 3, 4, 5, 6, 7, 8, 9);                    // 45

    // 8 long args -> also spills to overflow.
    r += sum_longs(8, 10L, 20L, 30L, 40L, 50L, 60L, 70L, 80L);      // 360

    // Pointer varargs: 'a' + 'b' + 'c' = 97 + 98 + 99 = 294.
    r += sum_first_bytes("abc", "b..", "c..", (const char *)0);     // 294

    return (int)(r % 256);   // (45 + 360 + 294) = 699 -> 699 % 256 = 187
}
