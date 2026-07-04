#ifndef __STDLIB_H__
#define __STDLIB_H__

// <stdlib.h> for the uvclang/UVM freestanding target: standard, strictly-typed C
// that clang lowers to LLVM IR. The allocator and exit() sit on top of the UVM
// syscalls in <uvm/syscalls.h>.
//
// This header is used ONLY for the UVM build (clang with -Iuvclang/include). The
// native reference build uses the platform's own libc, so the standard functions
// here are checked differentially against the real thing -- EXCEPT:
//   * rand()/srand(): a fixed MCG whose sequence differs from the host's, so its
//     raw output is not comparable to native.
//   * itoa()/ltoa(): non-standard, not present in the host libc.
// Both are still provided (correct on their own terms), just not diff-tested.

#include <stddef.h>          // size_t, NULL
#include <stdint.h>          // uint8_t / uint32_t / uint64_t / int8_t
#include <limits.h>          // LONG_MAX / LONG_MIN (strtol saturation)
#include <ctype.h>           // isspace (strtol leading whitespace)
#include <assert.h>          // assert (used by ltoa)
#include <uvm/syscalls.h>    // __uvm_exit / __uvm_vm_heap_size / __uvm_vm_grow_heap / __uvm_print_str

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

// We define RAND_MAX to be the same as INT32_MAX.
#define RAND_MAX 0x7FFFFFFF

// Every definition below (functions and the allocator/RNG state they own) carries
// weak linkage (UVCLANG_WEAK) so this header can be #included from any number of
// translation units without producing duplicate-symbol errors when they are
// linked together: LLVM keeps a single copy of each, so the allocator and RNG
// stay a single shared instance across the program. No per-file "implementation"
// opt-in is needed; in a single-translation-unit build the attribute has no
// effect and uvclang ignores it.
#ifndef UVCLANG_WEAK
#define UVCLANG_WEAK __attribute__((weak))
#endif

UVCLANG_WEAK int abs(int n)
{
    return n < 0 ? -n : n;
}

UVCLANG_WEAK long labs(long n)
{
    return n < 0 ? -n : n;
}

// End the program with the given status. <uvm/syscalls.h> defines exit() as a
// macro forwarding to the `exit` syscall; we replace it with the real libc
// function so &exit and standard usage both work. The status is truncated to 8
// bits by the syscall, matching a native process exit code (status & 0xFF).
#undef exit
UVCLANG_WEAK void exit(int status)
{
    __uvm_exit((int8_t)status);
    __builtin_unreachable();
}

// Convert a long to a string in the given base (2..16). Returns `str`.
UVCLANG_WEAK char *ltoa(long value, char *str, int base)
{
    assert(base >= 2 && base <= 16);

    char *start = str;

    // If negative, write a minus sign and continue with the magnitude.
    if (value < 0)
    {
        *str = '-';
        ++str;
        value = -value;
    }

    // Count the digits (at least one, for value == 0).
    int num_digits = 0;
    long n = value;
    do
    {
        n = n / base;
        ++num_digits;
    } while (n > 0);

    // The digits are produced least-significant first, so write them in reverse.
    for (int i = num_digits - 1; i >= 0; --i)
    {
        long digit = value % base;
        value = value / base;

        if (digit < 10)
            str[i] = (char)('0' + digit);
        else
            str[i] = (char)('A' + (digit - 10));
    }

    str[num_digits] = '\0';
    return start;
}

// Convert an int to a string in the given base (2..16). Returns `str`.
UVCLANG_WEAK char *itoa(int value, char *str, int base)
{
    return ltoa((long)value, str, base);
}

// --- string to integer conversion ------------------------------------------

// Parse a long from `str` in the given base (0 or 2..36). Leading whitespace is
// skipped; an optional +/- sign and, for base 0/16, an optional "0x" prefix are
// accepted; base 0 also auto-detects octal from a leading '0'. On success
// *endptr (when non-NULL) is left just past the last digit consumed; if no
// digits are converted it is set to `str`. On overflow the result saturates to
// LONG_MAX / LONG_MIN (standard strtol also sets errno=ERANGE, which this
// freestanding build has no errno for — only the clamped return value matches).
UVCLANG_WEAK long strtol(const char *str, char **endptr, int base)
{
    const char *s = str;

    // Leading whitespace.
    while (isspace((unsigned char)*s))
        ++s;

    // Optional sign.
    int neg = 0;
    if (*s == '+' || *s == '-')
    {
        neg = (*s == '-');
        ++s;
    }

    // "0x" prefix (only for base 0 or 16, and only when a hex digit follows).
    if ((base == 0 || base == 16) && s[0] == '0' &&
        (s[1] == 'x' || s[1] == 'X') && isxdigit((unsigned char)s[2]))
    {
        s += 2;
        base = 16;
    }
    else if (base == 0 && s[0] == '0')
        base = 8;      // leading 0 => octal; the loop consumes the '0' itself
    else if (base == 0)
        base = 10;

    // Saturation limit as an unsigned magnitude: LONG_MAX for '+', |LONG_MIN|
    // (== LONG_MAX + 1) for '-'.
    unsigned long limit  = neg ? (unsigned long)LONG_MAX + 1UL : (unsigned long)LONG_MAX;
    unsigned long cutoff = limit / (unsigned long)base;
    unsigned long cutlim = limit % (unsigned long)base;

    const char *digits = s;
    unsigned long acc = 0;
    int overflow = 0;
    for (;; ++s)
    {
        int d;
        char ch = *s;
        if (ch >= '0' && ch <= '9')      d = ch - '0';
        else if (ch >= 'a' && ch <= 'z') d = ch - 'a' + 10;
        else if (ch >= 'A' && ch <= 'Z') d = ch - 'A' + 10;
        else                             break;
        if (d >= base)
            break;

        // Keep scanning valid digits even past overflow, so endptr is correct.
        if (acc > cutoff || (acc == cutoff && (unsigned long)d > cutlim))
            overflow = 1;
        else
            acc = acc * (unsigned long)base + (unsigned long)d;
    }

    // No digits converted => endptr points at the original string.
    if (endptr != NULL)
        *endptr = (char *)((s == digits) ? str : s);

    if (overflow)
        return neg ? LONG_MIN : LONG_MAX;

    // Negate in unsigned space to avoid signed-overflow UB at LONG_MIN.
    return neg ? (long)(0UL - acc) : (long)acc;
}

// atoi is defined by the standard as (int)strtol(str, NULL, 10) apart from
// error handling (overflow is undefined, so it need not saturate).
UVCLANG_WEAK int atoi(const char *str)
{
    return (int)strtol(str, NULL, 10);
}

// --- pseudo-random numbers -------------------------------------------------
// Multiplicative Congruential Generator (MCG). Constant from "Computationally
// Easy, Spectrally Good Multipliers for Congruential Pseudorandom Number
// Generators" by Steele & Vigna:  x_n = (a * x_{n-1}) mod 2^64, a below. The
// state has 64 bits and the seed must be odd.
UVCLANG_WEAK unsigned long __uvclang_rand_state = 1337;

UVCLANG_WEAK int rand(void)
{
    __uvclang_rand_state = 0xf1357aea2e62a9c5UL * __uvclang_rand_state;
    // Use the upper 31 bits of the state only.
    return (int)(__uvclang_rand_state >> 33);
}

UVCLANG_WEAK void srand(unsigned int seed)
{
    // The seed must be odd, so force the lowest bit to 1.
    __uvclang_rand_state = ((unsigned long)seed << 1) + 1;
}

// --- bump allocator --------------------------------------------------------
// A simple grow-only bump allocator over the UVM heap. Each
// block is prefixed with an 8-byte header holding a magic word used to catch
// double-free / corruption. free() does not reclaim memory.

// Round `x` up to the next multiple of `n` (a power of two). 64-bit throughout
// so heap addresses above 4 GiB are handled correctly.
#define __uvclang_align_up(x, n) \
    (((uint64_t)(x) + ((uint64_t)(n) - 1)) & ~((uint64_t)(n) - 1))

UVCLANG_WEAK uint64_t __uvclang_heap_size = 0;   // current heap size in bytes
UVCLANG_WEAK uint8_t *__uvclang_next_alloc = 0;  // bump pointer into the heap

UVCLANG_WEAK void *malloc(size_t size)
{
    // On the first allocation, start the bump pointer at the top of the heap.
    if (__uvclang_next_alloc == 0)
    {
        __uvclang_heap_size = __uvm_vm_heap_size();
        __uvclang_next_alloc = (uint8_t *)__uvclang_heap_size;
    }

    // Reserve an 8-byte header followed by the user block, then bump (8-aligned).
    uint8_t *header_ptr = __uvclang_next_alloc;
    uint8_t *block_ptr = header_ptr + 8;
    __uvclang_next_alloc = (uint8_t *)__uvclang_align_up(block_ptr + size, 8);

    // Grow the heap if the bump pointer ran past the current end.
    if ((uint64_t)__uvclang_next_alloc > __uvclang_heap_size)
        __uvclang_heap_size = __uvm_vm_grow_heap((uint64_t)__uvclang_next_alloc);

    // Write a magic word into the header for safety checks.
    *(uint32_t *)header_ptr = 0x1337BAB3;

    return (void *)block_ptr;
}

UVCLANG_WEAK void free(void *ptr)
{
    if (ptr == NULL)
        return;

    // Verify the magic word; a mismatch means corruption or a double free.
    uint8_t *header_ptr = (uint8_t *)ptr - 8;
    uint32_t *magic_ptr = (uint32_t *)header_ptr;

    if (*magic_ptr != 0x1337BAB3)
    {
        __uvm_print_str("magic word does not match in free()\n");
        __uvm_exit(-1);
    }

    // Corrupt the magic word so a subsequent double free is detected.
    *magic_ptr = 0x11111111;
}

#endif // __STDLIB_H__
