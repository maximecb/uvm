#ifndef __STDIO_H__
#define __STDIO_H__

// <stdio.h> for the uvclang/UVM freestanding target: standard C that clang lowers
// to LLVM IR. The character and string output primitives are UVM syscalls, pulled
// in from <uvm/syscalls.h>.
//
// This header is used ONLY for the UVM build (clang with -Iuvclang/include). The
// native reference build in the test harness uses the platform's own libc, so
// these functions are checked differentially against the real thing.
//
// printf() is built on putchar via <stdarg.h> (clang lowers the variadic callee
// to the x86_64 va_list walk uvclang emulates — see gen_va_start / variadic.c).
// Supported conversions: %d %i %u %x %X %o %c %s %p %% and the floating-point
// %f/%F %e/%E %g/%G %a/%A, with flags (- 0 + space #), field width and precision (both
// literal and `*`), and the `l`/`ll` (tolerated `h`/`hh`/`z`/`t`/`j`/`L`) length
// modifiers. The integer/string conversions are matched byte-for-byte against
// native libc by tests/printf.c.
//
// Float args: a `%f`/`%e`/`%g` argument is a `double`, and uvclang doesn't model
// the x86_64 XMM vararg save area. It works anyway because uvclang packs every
// vararg into one linear 8-byte-slot buffer, so the double's raw bits sit exactly
// where an *integer* va_arg reads — we pull them out with `va_arg(ap, unsigned
// long)`, keeping the walk in sync, and format from the bits. Digits are produced
// with f64 arithmetic (see the floating-point section below): accurate to ~15-17
// significant digits, with the last digit occasionally an ULP off native at a
// rounding boundary. tests/printf_float.c checks the common range byte-for-byte
// and the precision-stressing cases tolerantly.
//
// Any other unsupported specifier is echoed verbatim and its argument left
// unconsumed — correct only if no further integer conversions follow it.

#include <stddef.h>         // size_t, NULL (from clang's freestanding header)
#include <stdarg.h>
#include <uvm/syscalls.h>   // __uvm_print_str / __uvm_print_endl / putchar / getchar / file_* macros

// The externally-visible functions below (and the FILE pool they own) carry weak
// linkage (UVCLANG_WEAK) so this header can be #included from any number of
// translation units without producing duplicate-symbol errors when they are
// linked together (LLVM keeps a single copy of each). No per-file
// "implementation" opt-in is needed; in a single-translation-unit build the
// attribute has no effect and uvclang ignores it. The `static` __pf_* helpers
// already have internal linkage, so they need no attribute.
#ifndef UVCLANG_WEAK
#define UVCLANG_WEAK __attribute__((weak))
#endif

#define EOF (-1)

// Write a string followed by a newline to standard output. Standard puts
// returns a non-negative value on success; we return 0. The
// differential harness compares the bytes written, not this return value.
UVCLANG_WEAK int puts(const char *str)
{
    __uvm_print_str(str);
    __uvm_print_endl();
    return 0;
}

// putchar / getchar are already provided as function-like macros by
// <uvm/syscalls.h> (mapping directly to the `putchar` / `getchar` syscalls), so
// we only define the fallbacks if those macros are somehow unavailable.
#ifndef putchar
UVCLANG_WEAK int putchar(int ch)
{
    return __uvm_putchar(ch);
}
#endif

#ifndef getchar
UVCLANG_WEAK int getchar(void)
{
    return __uvm_getchar();
}
#endif

// --- printf implementation ------------------------------------------------
//
// Internal helpers. They must NOT be named `__uvm_*`: uvclang lowers a call to
// any `@__uvm_<name>` as an inline `syscall <name>`, so such a name here would
// be miscompiled into a bogus syscall instead of a real call.

// Conversion flags parsed from `%[flags]...`.
enum { __PF_LEFT = 1, __PF_ZERO = 2, __PF_PLUS = 4, __PF_SPACE = 8, __PF_ALT = 16 };

// Output sink shared by printf/sprintf/snprintf. When `buf` is NULL, output
// goes to stdout (putchar); otherwise it is written into `buf` up to `cap-1`
// bytes (leaving room for a terminating NUL). `len` always counts the total
// number of characters the format *would* produce, so it doubles as printf's
// return value and snprintf's would-be length.
typedef struct {
    char *buf;
    unsigned long cap;
    unsigned long len;
} __pf_sink;

static void __pf_put(__pf_sink *o, char c)
{
    if (o->buf == 0)
        putchar(c);
    else if (o->len + 1 < o->cap)
        o->buf[o->len] = c;
    o->len++;
}

// Emit `n` copies of `c` (n <= 0 emits nothing).
static void __pf_pad(__pf_sink *o, char c, int n)
{
    for (int i = 0; i < n; i++)
        __pf_put(o, c);
}

// Emit a string honoring precision (max chars, prec < 0 = none), field width,
// and left-justify.
static void __pf_emit_str(__pf_sink *o, const char *s, int width, int prec, int flags)
{
    if (s == 0)
        s = "(null)";
    int len = 0;
    while (s[len] != '\0' && (prec < 0 || len < prec))
        len++;
    int pad = width > len ? width - len : 0;
    if (!(flags & __PF_LEFT))
        __pf_pad(o, ' ', pad);
    for (int i = 0; i < len; i++)
        __pf_put(o, s[i]);
    if (flags & __PF_LEFT)
        __pf_pad(o, ' ', pad);
}

// Emit an integer conversion: magnitude `mag`, `neg` sign, `base` (8/10/16),
// `upper` hex case, with precision (min digits), width, and flags applied the
// way C's printf specifies.
static void __pf_emit_int(__pf_sink *o, unsigned long mag, int neg, int base,
                          int upper, int width, int prec, int flags)
{
    char d[64];
    int dl = 0;
    const char *dg = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    if (mag == 0) {
        // A precision of 0 with value 0 produces *no* digits (C rule).
        if (prec != 0)
            d[dl++] = '0';
    } else {
        unsigned long v = mag;
        while (v != 0) { d[dl++] = dg[(unsigned int)(v % base)]; v /= base; }
    }
    int zprec = (prec > dl) ? prec - dl : 0;   // zero-fill up to the precision

    char sign = 0;
    if (neg)                     sign = '-';
    else if (flags & __PF_PLUS)  sign = '+';
    else if (flags & __PF_SPACE) sign = ' ';

    char pfx[2];
    int pl = 0;
    if ((flags & __PF_ALT) && base == 16 && mag != 0) {
        pfx[0] = '0';
        pfx[1] = upper ? 'X' : 'x';
        pl = 2;
    } else if ((flags & __PF_ALT) && base == 8) {
        // Octal alt form forces a leading 0 (unless the digits already start 0).
        if (zprec == 0 && (dl == 0 || d[dl - 1] != '0')) { pfx[0] = '0'; pl = 1; }
    }

    int content = (sign ? 1 : 0) + pl + zprec + dl;
    // '0' flag zero-pads to the width, but is ignored with '-' or an explicit
    // precision (for numeric conversions).
    int zpad = 0;
    if ((flags & __PF_ZERO) && !(flags & __PF_LEFT) && prec < 0 && width > content)
        zpad = width - content;
    int spad = (width > content + zpad) ? width - content - zpad : 0;

    if (!(flags & __PF_LEFT))
        __pf_pad(o, ' ', spad);
    if (sign)
        __pf_put(o, sign);
    for (int i = 0; i < pl; i++)
        __pf_put(o, pfx[i]);
    __pf_pad(o, '0', zpad);
    __pf_pad(o, '0', zprec);
    for (int i = dl; i > 0; i--)
        __pf_put(o, d[i - 1]);
    if (flags & __PF_LEFT)
        __pf_pad(o, ' ', spad);
}

// --- floating-point conversions (%f/%F, %e/%E, %g/%G) ---------------------
//
// UVM now has f64 arithmetic, so digits are produced in `double` (full ~15-17
// significant digits) rather than the old narrow-to-f32 path. There is no
// big-integer/dtoa machinery: values are scaled by a binary power-of-ten table
// and digits peeled off with f64 ops. Consequences, all differentially tested:
//   * %f/%e/%g are accurate to ~15-17 significant digits across the normal
//     range; the last digit can differ from native by an ULP at a rounding
//     boundary (table scaling is not sub-ULP exact).
//   * %f of |x| < 2^63 renders the integer part exactly (i64), so only the
//     fraction is subject to f64 rounding; larger magnitudes fall back to the
//     significant-digit path (correct leading ~17 digits, then zeros).

// Multiply `v` by 10^n (n may be negative) using a table of 10^(2^k). At most
// nine f64 mul/div; a few ULP of error (entries past 1e22 are the correctly-
// rounded doubles, not exact), which is why the last printed digit is not
// guaranteed bit-identical to native.
static double __pf_scale10(double v, int n)
{
    static const double p[] = { 1e1, 1e2, 1e4, 1e8, 1e16, 1e32, 1e64, 1e128, 1e256 };
    int neg = n < 0;
    if (neg) n = -n;
    for (int k = 0; k < 9 && n; k++, n >>= 1)
        if (n & 1) v = neg ? v / p[k] : v * p[k];
    return v;
}

// Produce `nsig` significant decimal digits of a positive finite double `v`
// into `d` (ASCII; d[0] is nonzero unless v == 0). Returns the decimal exponent
// E of the leading digit, i.e. v ~= d[0].d[1]d[2]... x 10^E. Rounds half-up at
// the last digit and renormalizes on a carry-out. `nsig` must be 1..18.
static int __pf_gen(double v, int nsig, char *d)
{
    if (v == 0.0) {
        for (int k = 0; k < nsig; k++) d[k] = '0';
        return 0;
    }
    // Estimate E from the binary exponent (log10(2) ~= 0.30103), then correct
    // the off-by-one(s) by direct comparison after scaling into [1, 10).
    union { double dd; unsigned long u; } pun;
    pun.dd = v;
    int be = (int)((pun.u >> 52) & 0x7FF) - 1023;
    int E = (int)((double)be * 0.30102999566398114);
    double s = __pf_scale10(v, -E);
    while (s >= 10.0) { s /= 10.0; E++; }
    while (s < 1.0)   { s *= 10.0; E--; }

    for (int k = 0; k < nsig; k++) {
        int dig = (int)s;
        if (dig < 0) dig = 0; else if (dig > 9) dig = 9;
        d[k] = (char)('0' + dig);
        s = (s - (double)dig) * 10.0;
    }
    // Round to nearest, ties to even, at the cut (s holds the next digit's
    // worth, in [0,10); an exact 5.0 is a true halfway case for a terminating
    // value). Ties resolve on the parity of the last kept digit, matching libc.
    int up = (s > 5.0) || (s == 5.0 && ((d[nsig - 1] - '0') & 1));
    if (up) {
        int k = nsig - 1;
        for (;;) {
            if (d[k] != '9') { d[k]++; break; }
            d[k] = '0';
            if (k == 0) { d[0] = '1'; E++; break; }  // 99..9 rounded up to 10..0
            k--;
        }
    }
    return E;
}

// Length of the "e+NN" exponent field (sign + at least two digits).
static int __pf_exp_len(int E)
{
    int ae = E < 0 ? -E : E, n = 0;
    while (ae) { n++; ae /= 10; }
    if (n < 2) n = 2;
    return n + 2;
}

// Emit the "e+NN" exponent field with `echar` ('e' or 'E').
static void __pf_put_exp(__pf_sink *o, int E, char echar)
{
    __pf_put(o, echar);
    __pf_put(o, E < 0 ? '-' : '+');
    int ae = E < 0 ? -E : E;
    char eb[8]; int el = 0;
    while (ae) { eb[el++] = (char)('0' + ae % 10); ae /= 10; }
    while (el < 2) eb[el++] = '0';
    for (int i = el; i > 0; i--) __pf_put(o, eb[i - 1]);
}

// Fixed-notation renderer. `d`/`avail` are the significant digits (positions at
// or beyond `avail` are implicit zeros), `E` the exponent of d[0], `fdigits` the
// number of fractional digits to print. Handles sign, width, zero-pad, justify.
static void __pf_render_fixed(__pf_sink *o, char sign, const char *d, int avail,
                              int E, int fdigits, int force_dot, int width, int flags)
{
    int intcount = (E >= 0) ? (E + 1) : 1;
    int has_dot = (fdigits > 0 || force_dot);
    int content = (sign ? 1 : 0) + intcount + (has_dot ? 1 : 0) + fdigits;
    int zpad = 0;
    if ((flags & __PF_ZERO) && !(flags & __PF_LEFT) && width > content)
        zpad = width - content;
    int spad = (width > content + zpad) ? width - content - zpad : 0;

    if (!(flags & __PF_LEFT)) __pf_pad(o, ' ', spad);
    if (sign) __pf_put(o, sign);
    __pf_pad(o, '0', zpad);
    if (E >= 0)
        for (int i = 0; i <= E; i++) __pf_put(o, (i < avail) ? d[i] : '0');
    else
        __pf_put(o, '0');
    if (has_dot) __pf_put(o, '.');
    for (int j = 0; j < fdigits; j++) {
        int k = E + 1 + j;                       // digit at fractional place j
        __pf_put(o, (k >= 0 && k < avail) ? d[k] : '0');
    }
    if (flags & __PF_LEFT) __pf_pad(o, ' ', spad);
}

// Scientific-notation renderer (shared by %e and %g's e-form).
static void __pf_render_sci(__pf_sink *o, char sign, const char *d, int avail,
                            int E, int fdigits, int force_dot, char echar,
                            int width, int flags)
{
    int has_dot = (fdigits > 0 || force_dot);
    int content = (sign ? 1 : 0) + 1 + (has_dot ? 1 : 0) + fdigits + __pf_exp_len(E);
    int zpad = 0;
    if ((flags & __PF_ZERO) && !(flags & __PF_LEFT) && width > content)
        zpad = width - content;
    int spad = (width > content + zpad) ? width - content - zpad : 0;

    if (!(flags & __PF_LEFT)) __pf_pad(o, ' ', spad);
    if (sign) __pf_put(o, sign);
    __pf_pad(o, '0', zpad);
    __pf_put(o, (0 < avail) ? d[0] : '0');
    if (has_dot) __pf_put(o, '.');
    for (int j = 1; j <= fdigits; j++) __pf_put(o, (j < avail) ? d[j] : '0');
    __pf_put_exp(o, E, echar);
    if (flags & __PF_LEFT) __pf_pad(o, ' ', spad);
}

// Emit a %f/%F/%e/%E/%g/%G conversion from the raw 64-bit `double` pattern
// `ubits`. Sign and inf/nan come straight from the IEEE-754 bit fields; the
// finite magnitude is formatted per `conv`.
static void __pf_emit_float(__pf_sink *o, unsigned long ubits, int width, int prec,
                            int flags, char conv)
{
    int upper = (conv == 'F' || conv == 'E' || conv == 'G');
    char echar = upper ? 'E' : 'e';

    int neg = (int)(ubits >> 63);
    char sign = neg ? '-' : (flags & __PF_PLUS) ? '+' : (flags & __PF_SPACE) ? ' ' : 0;
    unsigned expo = (unsigned)((ubits >> 52) & 0x7FF);

    // inf / nan: exponent field all ones; mantissa distinguishes them.
    if (expo == 0x7FF) {
        const char *s = (ubits & 0xFFFFFFFFFFFFFUL) ? (upper ? "NAN" : "nan")
                                                    : (upper ? "INF" : "inf");
        int len = 3 + (sign ? 1 : 0);
        int pad = width > len ? width - len : 0;
        if (!(flags & __PF_LEFT)) __pf_pad(o, ' ', pad);
        if (sign) __pf_put(o, sign);
        __pf_put(o, s[0]); __pf_put(o, s[1]); __pf_put(o, s[2]);
        if (flags & __PF_LEFT) __pf_pad(o, ' ', pad);
        return;
    }

    // %a / %A: exact hex float. Normalize to 1.f x 2^E2 (subnormals included),
    // emit the 52-bit mantissa as 13 hex nibbles. No scaling, so always exact.
    if (conv == 'a' || conv == 'A') {
        int up = (conv == 'A');
        const char *hexd = up ? "0123456789ABCDEF" : "0123456789abcdef";
        unsigned long mant = ubits & 0xFFFFFFFFFFFFFUL;   // 52-bit fraction
        int lead, E2;
        if (expo == 0) {
            if (mant == 0) { lead = 0; E2 = 0; }          // +-0 -> 0x0p+0
            else {                                        // subnormal: normalize
                int sh = 0;
                while (!(mant & 0x10000000000000UL)) { mant <<= 1; sh++; }
                mant &= 0xFFFFFFFFFFFFFUL;
                lead = 1; E2 = -1022 - sh;
            }
        } else {
            lead = 1; E2 = (int)expo - 1023;
        }

        char hd[13];
        for (int k = 0; k < 13; k++)
            hd[k] = hexd[(mant >> (48 - 4 * k)) & 0xF];
        int ndig = 13;
        if (prec >= 0 && prec < 13) {
            // Round the 52-bit fraction to `prec` nibbles, ties to even.
            int shift = 52 - 4 * prec;
            unsigned long kept = mant >> shift;
            unsigned long disc = mant & ((1UL << shift) - 1);
            unsigned long half = 1UL << (shift - 1);
            int rup = (disc > half) || (disc == half && (prec ? (kept & 1) : (lead & 1)));
            if (rup && ++kept >> (4 * prec)) { kept = 0; lead++; }  // carry to lead
            for (int k = 0; k < prec; k++)
                hd[k] = hexd[(kept >> (4 * (prec - 1 - k))) & 0xF];
            ndig = prec;
        } else if (prec < 0) {
            while (ndig > 0 && hd[ndig - 1] == '0') ndig--;   // trim trailing zeros
        }

        int eabs = E2 < 0 ? -E2 : E2, elen = 0;
        for (int t = eabs; ; t /= 10) { elen++; if (t < 10) break; }
        int has_dot = (ndig > 0 || (flags & __PF_ALT));
        int content = (sign ? 1 : 0) + 2 + 1 + (has_dot ? 1 : 0) + ndig + 2 + elen;
        int zpad = 0;
        if ((flags & __PF_ZERO) && !(flags & __PF_LEFT) && width > content)
            zpad = width - content;
        int spad = (width > content + zpad) ? width - content - zpad : 0;

        if (!(flags & __PF_LEFT)) __pf_pad(o, ' ', spad);
        if (sign) __pf_put(o, sign);
        __pf_put(o, '0'); __pf_put(o, up ? 'X' : 'x');
        __pf_pad(o, '0', zpad);
        __pf_put(o, hexd[lead]);
        if (has_dot) __pf_put(o, '.');
        for (int k = 0; k < ndig; k++) __pf_put(o, k < 13 ? hd[k] : '0');
        __pf_put(o, up ? 'P' : 'p');
        __pf_put(o, E2 < 0 ? '-' : '+');
        char eb[8]; int el = 0;
        for (int t = eabs; ; t /= 10) { eb[el++] = (char)('0' + t % 10); if (t < 10) break; }
        for (int k = el; k > 0; k--) __pf_put(o, eb[k - 1]);
        if (flags & __PF_LEFT) __pf_pad(o, ' ', spad);
        return;
    }

    // Finite magnitude as a double (sign bit cleared).
    union { unsigned long u; double d; } pun;
    pun.u = ubits & 0x7FFFFFFFFFFFFFFFUL;
    double v = pun.d;

    char d[20];

    if (conv == 'e' || conv == 'E') {
        if (prec < 0) prec = 6;
        int nsig = prec + 1;
        if (nsig > 18) nsig = 18;
        int E = __pf_gen(v, nsig, d);
        __pf_render_sci(o, sign, d, nsig, E, prec,
                        (prec > 0) || (flags & __PF_ALT), echar, width, flags);
        return;
    }

    if (conv == 'g' || conv == 'G') {
        if (prec < 0) prec = 6;
        if (prec == 0) prec = 1;
        int P = prec;
        if (P > 18) P = 18;
        int E = __pf_gen(v, P, d);
        int ndig = P;
        if (!(flags & __PF_ALT))                 // trim trailing zeros
            while (ndig > 1 && d[ndig - 1] == '0') ndig--;
        if (E < -4 || E >= P)                    // C's fixed-vs-scientific rule
            __pf_render_sci(o, sign, d, ndig, E, ndig - 1,
                            (flags & __PF_ALT), echar, width, flags);
        else
            __pf_render_fixed(o, sign, d, ndig, E,
                              (E + 1 <= ndig ? ndig - (E + 1) : 0),
                              (flags & __PF_ALT), width, flags);
        return;
    }

    // %f / %F.
    if (prec < 0) prec = 6;
    if (v < 9223372036854775808.0) {             // < 2^63: exact integer part
        unsigned long ip = (unsigned long)v;     // floor (v >= 0)
        double frac = v - (double)ip;

        // Fraction digits. A double resolves ~17 significant digits, so only the
        // first 18 fractional places can be nonzero here; the rest print as '0'.
        int fcomp = prec < 18 ? prec : 18;
        char fd[20];
        for (int k = 0; k < fcomp; k++) {
            frac *= 10.0;
            int dg = (int)frac;
            if (dg > 9) dg = 9; else if (dg < 0) dg = 0;
            fd[k] = (char)('0' + dg);
            frac -= (double)dg;
        }
        // Round to nearest, ties to even, at the printed precision, then carry.
        // (A precision past the computed run sees a zero tail and never rounds.)
        int up = 0;
        if (fcomp == prec) {
            if (frac > 0.5) up = 1;
            else if (frac == 0.5) {
                int last = prec > 0 ? (fd[prec - 1] - '0') : (int)(ip % 10UL);
                up = last & 1;
            }
        }
        if (up) {
            int k = fcomp - 1;
            while (k >= 0 && fd[k] == '9') { fd[k] = '0'; k--; }
            if (k >= 0) fd[k]++;
            else ip++;                            // 9.99..->10.00..: carry up
        }

        char id[24];
        int il = 0;
        if (ip == 0) id[il++] = '0';
        else { unsigned long t = ip; while (t) { id[il++] = (char)('0' + (int)(t % 10UL)); t /= 10UL; } }

        int has_dot = (prec > 0 || (flags & __PF_ALT));
        int content = (sign ? 1 : 0) + il + (has_dot ? 1 : 0) + prec;
        int zpad = 0;
        if ((flags & __PF_ZERO) && !(flags & __PF_LEFT) && width > content)
            zpad = width - content;
        int spad = (width > content + zpad) ? width - content - zpad : 0;

        if (!(flags & __PF_LEFT)) __pf_pad(o, ' ', spad);
        if (sign) __pf_put(o, sign);
        __pf_pad(o, '0', zpad);
        for (int i = il; i > 0; i--) __pf_put(o, id[i - 1]);
        if (has_dot) __pf_put(o, '.');
        for (int k = 0; k < prec; k++) __pf_put(o, k < fcomp ? fd[k] : '0');
        if (flags & __PF_LEFT) __pf_pad(o, ' ', spad);
    } else {
        // |x| >= 2^63: integral, beyond i64. Use the significant-digit path;
        // digits past the 17th print as zeros (the double has no more).
        int E = __pf_gen(v, 17, d);
        __pf_render_fixed(o, sign, d, 17, E, prec,
                          (prec > 0 || (flags & __PF_ALT)), width, flags);
    }
}

// Read a base-10 field (width/precision) from the format string, advancing *pi.
static int __pf_num(const char *fmt, int *pi)
{
    int n = 0;
    while (fmt[*pi] >= '0' && fmt[*pi] <= '9') { n = n * 10 + (fmt[*pi] - '0'); (*pi)++; }
    return n;
}

// The core format engine, shared by every printf/sprintf variant.
static void __pf_format(__pf_sink *o, const char *fmt, va_list ap)
{
    for (int i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] != '%') {
            __pf_put(o, fmt[i]);
            continue;
        }

        i++;  // consume '%'

        // Flags.
        int flags = 0;
        for (;;) {
            char f = fmt[i];
            if (f == '-')      flags |= __PF_LEFT;
            else if (f == '0') flags |= __PF_ZERO;
            else if (f == '+') flags |= __PF_PLUS;
            else if (f == ' ') flags |= __PF_SPACE;
            else if (f == '#') flags |= __PF_ALT;
            else break;
            i++;
        }

        // Field width ('*' takes it from the arg list; negative => left-justify).
        int width = 0;
        if (fmt[i] == '*') {
            width = va_arg(ap, int);
            if (width < 0) { flags |= __PF_LEFT; width = -width; }
            i++;
        } else {
            width = __pf_num(fmt, &i);
        }

        // Precision ('.' then digits or '*'; a bare '.' means precision 0).
        int prec = -1;
        if (fmt[i] == '.') {
            i++;
            if (fmt[i] == '*') { prec = va_arg(ap, int); if (prec < 0) prec = -1; i++; }
            else prec = __pf_num(fmt, &i);
        }

        // Length modifiers: `l`/`ll` select 64-bit; others (h/hh/z/t/j) are
        // tolerated and ignored (their args are still passed as int/long).
        int longf = 0;
        while (fmt[i] == 'l') { longf++; i++; }
        while (fmt[i] == 'h' || fmt[i] == 'z' || fmt[i] == 't' || fmt[i] == 'j' || fmt[i] == 'L') i++;

        char c = fmt[i];
        switch (c) {
            case '\0':  // malformed spec at end of string: emit '%' and stop
                __pf_put(o, '%'); i--; break;
            case '%': __pf_put(o, '%'); break;
            case 'c': {
                char ch = (char)va_arg(ap, int);
                int pad = width > 1 ? width - 1 : 0;
                if (!(flags & __PF_LEFT)) __pf_pad(o, ' ', pad);
                __pf_put(o, ch);
                if (flags & __PF_LEFT) __pf_pad(o, ' ', pad);
                break;
            }
            case 's':
                __pf_emit_str(o, va_arg(ap, const char *), width, prec, flags);
                break;
            case 'd':
            case 'i': {
                long v = longf ? va_arg(ap, long) : (long)va_arg(ap, int);
                int neg = v < 0;
                unsigned long mag = neg ? -(unsigned long)v : (unsigned long)v;
                __pf_emit_int(o, mag, neg, 10, 0, width, prec, flags);
                break;
            }
            case 'u': {
                unsigned long v = longf ? va_arg(ap, unsigned long)
                                        : (unsigned long)va_arg(ap, unsigned int);
                __pf_emit_int(o, v, 0, 10, 0, width, prec, flags);
                break;
            }
            case 'o': {
                unsigned long v = longf ? va_arg(ap, unsigned long)
                                        : (unsigned long)va_arg(ap, unsigned int);
                __pf_emit_int(o, v, 0, 8, 0, width, prec, flags);
                break;
            }
            case 'x': {
                unsigned long v = longf ? va_arg(ap, unsigned long)
                                        : (unsigned long)va_arg(ap, unsigned int);
                __pf_emit_int(o, v, 0, 16, 0, width, prec, flags);
                break;
            }
            case 'X': {
                unsigned long v = longf ? va_arg(ap, unsigned long)
                                        : (unsigned long)va_arg(ap, unsigned int);
                __pf_emit_int(o, v, 0, 16, 1, width, prec, flags);
                break;
            }
            case 'p': {
                // Pointer: "0x" + hex. Not differentially testable (the address
                // itself differs between native and UVM).
                __pf_put(o, '0'); __pf_put(o, 'x');
                __pf_emit_int(o, (unsigned long)va_arg(ap, void *), 0, 16, 0, 0, -1, 0);
                break;
            }
            case 'f':
            case 'F':
            case 'e':
            case 'E':
            case 'g':
            case 'G':
            case 'a':
            case 'A':
                // A `double` arg. We can't use the XMM save area, but its 64 bits
                // sit in the linear vararg buffer where an integer va_arg reads,
                // so pull them out through the GP path (this also keeps the walk
                // in sync for any following conversions). See the file header.
                __pf_emit_float(o, va_arg(ap, unsigned long), width, prec, flags, c);
                break;
            default:
                // Unknown/unsupported specifier: echo it verbatim and do NOT
                // consume a vararg — we couldn't interpret its slot.
                __pf_put(o, '%');
                __pf_put(o, c);
                break;
        }
    }
}

UVCLANG_WEAK int vprintf(const char *fmt, va_list ap)
{
    __pf_sink o = { 0, 0, 0 };
    __pf_format(&o, fmt, ap);
    return (int)o.len;
}

UVCLANG_WEAK int printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int count = vprintf(fmt, ap);
    va_end(ap);
    return count;
}

// Write into `buf` at most `size-1` chars plus a terminating NUL. Returns the
// number of characters that *would* have been written (excluding the NUL), like
// C99 snprintf.
UVCLANG_WEAK int vsnprintf(char *buf, unsigned long size, const char *fmt, va_list ap)
{
    __pf_sink o = { buf, size, 0 };
    __pf_format(&o, fmt, ap);
    if (size > 0)
        buf[o.len < size ? o.len : size - 1] = '\0';
    return (int)o.len;
}

UVCLANG_WEAK int snprintf(char *buf, unsigned long size, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return n;
}

// Unbounded sprintf: a snprintf whose buffer is treated as effectively infinite.
UVCLANG_WEAK int vsprintf(char *buf, const char *fmt, va_list ap)
{
    return vsnprintf(buf, (unsigned long)-1, fmt, ap);
}

UVCLANG_WEAK int sprintf(char *buf, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, (unsigned long)-1, fmt, ap);
    va_end(ap);
    return n;
}

// --- File streams (FILE*) --------------------------------------------------
//
// UVM has no buffered stdio, so these are thin, unbuffered wrappers over the
// file_* syscalls in <uvm/syscalls.h>. A FILE holds the UVM file handle plus
// the sticky end-of-file / error indicators reported by feof()/ferror().
//
// Streams are handed out from a small fixed pool rather than malloc: fclose()
// can then return a slot for reuse (the <stdlib.h> bump allocator never frees),
// and file I/O works without pulling in the allocator. FOPEN_MAX caps the
// number of simultaneously open streams.
//
// All access is binary: a 'b' in the mode string is accepted and ignored, which
// matches the byte-exact (no newline translation) semantics of the UVM file
// syscalls. Since native libc opened in binary mode behaves identically, the
// read/write/seek/tell paths are checked differentially by tests/file_io.c.

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#define FOPEN_MAX 32

typedef struct {
    uint64_t __handle;   // UVM file handle; 0 marks a free pool slot
    int __eof;           // sticky end-of-file indicator (feof)
    int __error;         // sticky I/O error indicator (ferror)
} FILE;

static FILE __uvclang_files[FOPEN_MAX];

// Open `path` per the C mode string. The first character selects the primary
// mode (r/w/a); a '+' anywhere adds the opposite access; 'b' is ignored (all
// UVM I/O is binary). Returns a stream, or NULL on a bad mode, a rejected/absent
// path, or an exhausted stream pool.
UVCLANG_WEAK FILE *fopen(const char *path, const char *mode)
{
    uint64_t flags;
    int append = 0;
    switch (mode[0]) {
        case 'r': flags = OPEN_READ; break;
        case 'w': flags = OPEN_WRITE | OPEN_CREATE | OPEN_TRUNC; break;
        case 'a': flags = OPEN_WRITE | OPEN_CREATE; append = 1; break;
        default:  return NULL;
    }
    for (int i = 1; mode[i] != '\0'; i++)
        if (mode[i] == '+')
            flags |= OPEN_READ | OPEN_WRITE;

    uint64_t handle = file_open(path, flags);
    if (handle == 0)
        return NULL;

    // Claim a free slot (handle == 0). Fail closed if the pool is full.
    FILE *fp = NULL;
    for (int i = 0; i < FOPEN_MAX; i++) {
        if (__uvclang_files[i].__handle == 0) {
            fp = &__uvclang_files[i];
            break;
        }
    }
    if (fp == NULL) {
        file_close(handle);
        return NULL;
    }

    fp->__handle = handle;
    fp->__eof = 0;
    fp->__error = 0;

    // Append mode starts positioned at end of file.
    if (append)
        file_seek(handle, file_size(handle));

    return fp;
}

// Close `stream` and return its pool slot. Returns 0 on success, EOF if the
// stream is NULL.
UVCLANG_WEAK int fclose(FILE *stream)
{
    if (stream == NULL)
        return EOF;
    file_close(stream->__handle);
    stream->__handle = 0;   // release the slot back to the pool
    return 0;
}

// Read up to `nmemb` elements of `size` bytes into `ptr`. Loops over file_read
// so a short syscall read (fewer bytes than asked, without EOF) still fills the
// request. Returns the number of *complete* elements read; a partial final
// element (at EOF) is not counted, matching C fread. Sets the EOF/error flag on
// a short read / failure.
UVCLANG_WEAK size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    if (size == 0 || nmemb == 0)
        return 0;
    size_t total = size * nmemb;
    uint8_t *p = (uint8_t *)ptr;
    size_t got = 0;
    while (got < total) {
        long n = file_read(stream->__handle, p + got, total - got);
        if (n < 0) { stream->__error = 1; break; }
        if (n == 0) { stream->__eof = 1; break; }
        got += (size_t)n;
    }
    return got / size;
}

// Write `nmemb` elements of `size` bytes from `ptr`. Loops over file_write to
// push out any bytes a single syscall left behind. Returns the number of
// complete elements written; sets the error flag on failure.
UVCLANG_WEAK size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    if (size == 0 || nmemb == 0)
        return 0;
    size_t total = size * nmemb;
    const uint8_t *p = (const uint8_t *)ptr;
    size_t put = 0;
    while (put < total) {
        long n = file_write(stream->__handle, p + put, total - put);
        if (n < 0) { stream->__error = 1; break; }
        if (n == 0) break;
        put += (size_t)n;
    }
    return put / size;
}

// Reposition the stream. UVM's file_seek is absolute-only, so SEEK_CUR/SEEK_END
// are resolved against file_tell/file_size here. A successful seek clears the
// end-of-file indicator (C semantics). Returns 0 on success, -1 on a bad
// `whence` or a resulting negative offset.
UVCLANG_WEAK int fseek(FILE *stream, long offset, int whence)
{
    long base;
    switch (whence) {
        case SEEK_SET: base = 0; break;
        case SEEK_CUR: base = (long)file_tell(stream->__handle); break;
        case SEEK_END: base = (long)file_size(stream->__handle); break;
        default:       return -1;
    }
    long pos = base + offset;
    if (pos < 0)
        return -1;
    file_seek(stream->__handle, (uint64_t)pos);
    stream->__eof = 0;
    return 0;
}

// Current byte offset from the start of the file, or -1 on error.
UVCLANG_WEAK long ftell(FILE *stream)
{
    return (long)file_tell(stream->__handle);
}

// Nonzero once a read has hit end of file (cleared by fseek).
UVCLANG_WEAK int feof(FILE *stream)
{
    return stream->__eof;
}

// Nonzero once an I/O error has occurred on the stream.
UVCLANG_WEAK int ferror(FILE *stream)
{
    return stream->__error;
}

// Clear the end-of-file and error indicators.
UVCLANG_WEAK void clearerr(FILE *stream)
{
    stream->__eof = 0;
    stream->__error = 0;
}

#endif // __STDIO_H__
