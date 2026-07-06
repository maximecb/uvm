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

// --- free-list allocator ---------------------------------------------------
// A segregated free-list allocator with boundary-tag coalescing over the UVM
// heap. Unlike a bump allocator, free() reclaims memory: freed chunks are
// coalesced with any free neighbours and kept on size-class bins for reuse. A
// single global spin lock (one atomic word) makes malloc/free/calloc/realloc
// thread-safe; critical sections are short, so a plain spin is adequate.
//
// Chunk layout (every chunk size is a multiple of 8, so the low 3 header bits
// are free for flags):
//
//     +0  uint64 header:  chunk size | INUSE (bit0) | PINUSE (bit1)
//     +8  payload  <-- malloc returns this address
//         When the chunk is free, the payload's first 16 bytes hold the fd/bk
//         bin links, and the chunk's last 8 bytes hold a footer copy of the
//         size so the chunk above can find this one's header (backward
//         coalescing).
//
// INUSE is this chunk's own state; PINUSE is the *previous* (lower-address)
// chunk's state. A free chunk writes a footer, an in-use one does not, so
// PINUSE is what tells free() whether it may read the footer below. The
// highest-address chunk is the "top" (wilderness): it borders the unallocated
// grown heap and is extended via __uvm_vm_grow_heap instead of being binned.

// Round `x` up to the next multiple of `n` (a power of two). 64-bit throughout
// so heap addresses above 4 GiB are handled correctly.
#define __uvclang_align_up(x, n) \
    (((uint64_t)(x) + ((uint64_t)(n) - 1)) & ~((uint64_t)(n) - 1))

#include <stdatomic.h>       // _Atomic / atomic_* for the allocator's spin lock

#define __UVCLANG_MIN_CHUNK 32u        // header(8) + fd(8) + bk(8) + footer(8)
#define __UVCLANG_NBINS     48
#define __UVCLANG_GROW_MIN  65536u     // grow the heap by at least this much

#define __uvclang_hdr(c)    (*(uint64_t *)(c))
#define __uvclang_size(c)   (__uvclang_hdr(c) & ~(uint64_t)7)
#define __uvclang_inuse(c)  (__uvclang_hdr(c) & 1u)
#define __uvclang_pinuse(c) (__uvclang_hdr(c) & 2u)
#define __uvclang_fd(c)     (*(uint8_t **)((c) + 8))   // free-list forward link
#define __uvclang_bk(c)     (*(uint8_t **)((c) + 16))  // free-list back link

UVCLANG_WEAK uint8_t *__uvclang_bins[__UVCLANG_NBINS] = {0};
UVCLANG_WEAK uint8_t *__uvclang_top = 0;        // wilderness chunk (0 = uninit)
UVCLANG_WEAK uint64_t __uvclang_heap_base = 0;  // first usable heap address
UVCLANG_WEAK uint64_t __uvclang_heap_end = 0;   // current heap size (end addr)
UVCLANG_WEAK _Atomic uint64_t __uvclang_malloc_lock = 0;

// Acquire/release the global allocator lock. Spins on a CAS; the guest threads
// are real and preemptively scheduled, so a holder always makes progress.
static inline void __uvclang_lock(void)
{
    uint64_t expected = 0;
    while (!atomic_compare_exchange_weak(&__uvclang_malloc_lock, &expected, 1))
        expected = 0;
}
static inline void __uvclang_unlock(void)
{
    atomic_store(&__uvclang_malloc_lock, 0);
}

static inline void __uvclang_die(const char *msg)
{
    __uvm_print_str(msg);
    __uvm_exit(-1);
}

// Write a chunk header from its parts.
static inline void __uvclang_set_hdr(uint8_t *c, uint64_t size, int inuse, uint64_t pinuse)
{
    __uvclang_hdr(c) = size | (inuse ? 1u : 0u) | (pinuse ? 2u : 0u);
}

// Smallest bin that could hold a chunk of `size`. Monotonic non-decreasing in
// size, so allocation can search this bin and every larger one. Sizes up to 280
// bytes get an exact class; above that, bins cover power-of-two ranges.
static inline int __uvclang_bin_index(uint64_t size)
{
    uint64_t n = size >> 3;                     // size in 8-byte units, >= 4
    if (n < 36)
        return (int)(n - 4);                    // bins 0..31: exact classes
    int idx = 32;
    while (n >= 64 && idx < __UVCLANG_NBINS - 1) { n >>= 1; ++idx; }
    return idx;                                 // bins 32..47: log-size ranges
}

// Insert/remove a free chunk at the head of its size-class bin (doubly linked).
static inline void __uvclang_bin_insert(uint8_t *c)
{
    int b = __uvclang_bin_index(__uvclang_size(c));
    uint8_t *head = __uvclang_bins[b];
    __uvclang_fd(c) = head;
    __uvclang_bk(c) = 0;
    if (head)
        __uvclang_bk(head) = c;
    __uvclang_bins[b] = c;
}
static inline void __uvclang_bin_remove(uint8_t *c)
{
    uint8_t *fd = __uvclang_fd(c), *bk = __uvclang_bk(c);
    if (bk)
        __uvclang_fd(bk) = fd;
    else
        __uvclang_bins[__uvclang_bin_index(__uvclang_size(c))] = fd;
    if (fd)
        __uvclang_bk(fd) = bk;
}

// Round a user request up to a whole chunk size (header + payload, 8-aligned,
// clamped to the minimum). Returns 0 on size overflow.
static inline uint64_t __uvclang_chunk_size(size_t req)
{
    uint64_t s = (uint64_t)req + 8;
    if (s < (uint64_t)req)                      // 8-byte header addition wrapped
        return 0;
    s = __uvclang_align_up(s, 8);
    if (s < __UVCLANG_MIN_CHUNK)
        s = __UVCLANG_MIN_CHUNK;
    return s;
}

// One-time setup: seed the top (wilderness) chunk at the current heap end and
// grow the heap once so the first allocations don't each hit a syscall.
UVCLANG_WEAK void __uvclang_malloc_init(void)
{
    __uvclang_heap_base = __uvm_vm_heap_size();
    __uvclang_heap_end  = __uvm_vm_grow_heap(__uvclang_heap_base + __UVCLANG_GROW_MIN);
    __uvclang_top = (uint8_t *)__uvclang_heap_base;
    // PINUSE=1: nothing lies below the base, so free() must never coalesce down.
    __uvclang_set_hdr(__uvclang_top, __uvclang_heap_end - __uvclang_heap_base, 0, 2);
}

// Core allocator. Assumes the lock is held.
UVCLANG_WEAK void *__uvclang_malloc_nolock(size_t req)
{
    if (__uvclang_top == 0)
        __uvclang_malloc_init();

    uint64_t need = __uvclang_chunk_size(req);
    if (need == 0)
        return NULL;

    // First fit across the bins, smallest adequate size class first.
    for (int b = __uvclang_bin_index(need); b < __UVCLANG_NBINS; ++b)
    {
        for (uint8_t *c = __uvclang_bins[b]; c; c = __uvclang_fd(c))
        {
            uint64_t s = __uvclang_size(c);
            if (s < need)
                continue;
            __uvclang_bin_remove(c);
            uint64_t rem = s - need;
            if (rem >= __UVCLANG_MIN_CHUNK)
            {
                // Split: keep `need` here, return the tail to its bin. The chunk
                // above still sees a free predecessor, so its PINUSE is unchanged.
                __uvclang_set_hdr(c, need, 1, __uvclang_pinuse(c));
                uint8_t *rc = c + need;
                __uvclang_set_hdr(rc, rem, 0, 2);          // prev (c) now in use
                *(uint64_t *)(rc + rem - 8) = rem;         // footer
                __uvclang_bin_insert(rc);
            }
            else
            {
                // Take the whole chunk; tell the next chunk its prev is in use.
                __uvclang_set_hdr(c, s, 1, __uvclang_pinuse(c));
                __uvclang_hdr(c + s) |= 2u;
            }
            return c + 8;
        }
    }

    // Nothing reusable: carve from the top, growing the heap if it is too small.
    uint8_t *top = __uvclang_top;
    if (__uvclang_size(top) < need + __UVCLANG_MIN_CHUNK)
    {
        uint64_t want = (uint64_t)top + need + __UVCLANG_MIN_CHUNK;
        if (want < __uvclang_heap_end + __UVCLANG_GROW_MIN)
            want = __uvclang_heap_end + __UVCLANG_GROW_MIN;
        __uvclang_heap_end = __uvm_vm_grow_heap(want);
        __uvclang_set_hdr(top, __uvclang_heap_end - (uint64_t)top, 0, __uvclang_pinuse(top));
    }
    __uvclang_set_hdr(top, need, 1, __uvclang_pinuse(top));
    uint8_t *newtop = top + need;
    __uvclang_set_hdr(newtop, __uvclang_heap_end - (uint64_t)newtop, 0, 2); // prev in use
    __uvclang_top = newtop;
    return top + 8;
}

// Core free. Assumes the lock is held.
UVCLANG_WEAK void __uvclang_free_nolock(void *ptr)
{
    uint8_t *c = (uint8_t *)ptr - 8;

    // Reject obviously bad pointers and (via the INUSE bit) double frees.
    if (__uvclang_top == 0 ||
        (uint64_t)c < __uvclang_heap_base ||
        (uint64_t)ptr >= __uvclang_heap_end ||
        ((uintptr_t)ptr & 7u) != 0 ||
        !__uvclang_inuse(c))
        __uvclang_die("invalid or double free() detected\n");

    __uvclang_hdr(c) &= ~(uint64_t)1;               // clear INUSE
    uint64_t size = __uvclang_size(c);
    int merged_top = 0;

    // Coalesce forward through any free neighbours, and into top if adjacent.
    uint8_t *next = c + size;
    for (;;)
    {
        if (next == __uvclang_top)
        {
            size += __uvclang_size(__uvclang_top);
            merged_top = 1;
            break;
        }
        if (__uvclang_inuse(next))
            break;
        __uvclang_bin_remove(next);
        size += __uvclang_size(next);
        next = c + size;
    }

    // Coalesce backward if the previous chunk is free (its footer holds its size).
    if (!__uvclang_pinuse(c))
    {
        uint64_t prevsize = *(uint64_t *)(c - 8);
        uint8_t *prev = c - prevsize;
        __uvclang_bin_remove(prev);
        size += prevsize;
        c = prev;                                   // PINUSE(prev) is preserved below
    }

    if (merged_top)
    {
        __uvclang_set_hdr(c, size, 0, __uvclang_pinuse(c));
        __uvclang_top = c;                          // top carries no footer/bin link
    }
    else
    {
        __uvclang_set_hdr(c, size, 0, __uvclang_pinuse(c));
        __uvclang_hdr(c + size) &= ~(uint64_t)2;    // next chunk: prev now free
        *(uint64_t *)(c + size - 8) = size;         // footer
        __uvclang_bin_insert(c);
    }
}

UVCLANG_WEAK void *malloc(size_t size)
{
    __uvclang_lock();
    void *p = __uvclang_malloc_nolock(size);
    __uvclang_unlock();
    return p;
}

UVCLANG_WEAK void free(void *ptr)
{
    if (ptr == NULL)
        return;
    __uvclang_lock();
    __uvclang_free_nolock(ptr);
    __uvclang_unlock();
}

UVCLANG_WEAK void *calloc(size_t nmemb, size_t size)
{
    // Reject an overflowing nmemb*size *before* forming the product: computing
    // it first and dividing back is the idiom the optimizer folds to
    // llvm.umul.with.overflow, an intrinsic this backend does not implement.
    if (nmemb != 0 && size > (size_t)-1 / nmemb)
        return NULL;
    uint64_t total = (uint64_t)nmemb * (uint64_t)size;

    __uvclang_lock();
    void *p = __uvclang_malloc_nolock((size_t)total);
    __uvclang_unlock();

    if (p != NULL)
        __uvm_memset((uint8_t *)p, 0, total);
    return p;
}

UVCLANG_WEAK void *realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
        return malloc(size);
    if (size == 0)
    {
        free(ptr);
        return NULL;
    }

    __uvclang_lock();

    uint8_t *c = (uint8_t *)ptr - 8;
    uint64_t old = __uvclang_size(c);
    uint64_t need = __uvclang_chunk_size(size);

    // Fits in place: shrink, splitting off the tail if it is worth a chunk.
    if (need != 0 && old >= need)
    {
        uint64_t rem = old - need;
        if (rem >= __UVCLANG_MIN_CHUNK)
        {
            __uvclang_set_hdr(c, need, 1, __uvclang_pinuse(c));
            uint8_t *rc = c + need;
            __uvclang_set_hdr(rc, rem, 1, 2);       // mark in use, then free to coalesce
            __uvclang_free_nolock(rc + 8);
        }
        __uvclang_unlock();
        return ptr;
    }

    // Otherwise allocate a fresh block, copy the payload, and free the old one.
    void *np = __uvclang_malloc_nolock(size);
    if (np != NULL)
    {
        uint64_t copy = old - 8;                    // old payload byte count
        if ((uint64_t)size < copy)
            copy = (uint64_t)size;
        __uvm_memcpy((uint8_t *)np, (uint8_t *)ptr, copy);
        __uvclang_free_nolock(ptr);
    }
    __uvclang_unlock();
    return np;
}

#endif // __STDLIB_H__
