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
// Supported conversions: %d %i %u %x %X %o %c %s %p %f %F %%, with flags
// (- 0 + space #), field width and precision (both literal and `*`), and the
// `l`/`ll` (tolerated `h`/`hh`/`z`/`t`/`j`) length modifiers — matched
// byte-for-byte against native libc by tests/printf.c / tests/printf_float.c.
//
// Float support is single precision only. UVM has no f64, so a `float` vararg —
// which the ABI promotes to `double` — is de-promoted by uvclang (fpext keeps
// the f32 bits in the low 32 of the slot) and read back here as those f32 bits,
// then formatted with f32 arithmetic (see __pf_emit_float). Exactly
// representable values print exactly; others may differ from a true double `%f`
// in the low digits, and |x| >= 2^31 is out of range. Reading the slot as an
// integer `va_arg` keeps the FP arg in step with the plain GP va_list walk, so
// conversions after a %f stay correct (no XMM save area needed).
//
// Still NOT supported: %e / %g (and any genuine `double` value that isn't a
// promoted float). An unsupported specifier is echoed verbatim and its argument
// left unconsumed — correct only if no further conversions follow it.

#include <stdarg.h>
#include <uvm/syscalls.h>   // __uvm_print_str / __uvm_print_endl / putchar / getchar macros

#define EOF (-1)

// Write a string followed by a newline to standard output. Standard puts
// returns a non-negative value on success; we return 0. The
// differential harness compares the bytes written, not this return value.
int puts(const char *str)
{
    __uvm_print_str(str);
    __uvm_print_endl();
    return 0;
}

// putchar / getchar are already provided as function-like macros by
// <uvm/syscalls.h> (mapping directly to the `putchar` / `getchar` syscalls), so
// we only define the fallbacks if those macros are somehow unavailable.
#ifndef putchar
int putchar(int ch)
{
    return __uvm_putchar(ch);
}
#endif

#ifndef getchar
int getchar(void)
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

// Read a base-10 field (width/precision) from the format string, advancing *pi.
static int __pf_num(const char *fmt, int *pi)
{
    int n = 0;
    while (fmt[*pi] >= '0' && fmt[*pi] <= '9') { n = n * 10 + (fmt[*pi] - '0'); (*pi)++; }
    return n;
}

// Emit a `%f`/`%F` conversion for the float `x`. UVM has no f64, so a `float`
// vararg (promoted to `double` by the ABI) is passed and read as its low 32
// f32 bits — see __pf_format's `%f` case — and formatted here in f32 arithmetic.
// Results therefore carry single precision (~7 significant digits): exactly
// representable values print exactly, others may differ from a double `%f` in
// the low digits. Handles the +/space/#/0/- flags, field width, precision
// (default 6), and inf/nan. Values with |x| >= 2^31 exceed the 32-bit integer
// extraction and are out of range for this implementation.
static void __pf_emit_float(__pf_sink *o, float x, int width, int prec,
                            int flags, int upper)
{
    if (prec < 0)
        prec = 6;

    union { unsigned int u; float f; } bc;
    bc.f = x;
    int neg = (int)(bc.u >> 31);
    unsigned int absbits = bc.u & 0x7fffffffu;

    char sign = neg ? '-'
              : (flags & __PF_PLUS)  ? '+'
              : (flags & __PF_SPACE) ? ' ' : 0;

    // inf / nan: exponent field all ones. C prints NaN without a sign.
    if (absbits >= 0x7f800000u) {
        int isnan = absbits != 0x7f800000u;
        const char *w = isnan ? (upper ? "NAN" : "nan") : (upper ? "INF" : "inf");
        char sg = isnan ? 0 : sign;
        int wl = 3 + (sg ? 1 : 0);
        int pad = width > wl ? width - wl : 0;
        if (!(flags & __PF_LEFT)) __pf_pad(o, ' ', pad);
        if (sg) __pf_put(o, sg);
        for (int k = 0; w[k] != '\0'; k++) __pf_put(o, w[k]);
        if (flags & __PF_LEFT) __pf_pad(o, ' ', pad);
        return;
    }

    // Work with the magnitude as a positive float.
    bc.u = absbits;
    float m = bc.f;

    // Split into an integer part (fits in 32 bits for the supported range) and
    // a fraction, then peel `prec` decimal digits, rounding half away from zero.
    unsigned int ip = (unsigned int)m;      // truncates toward zero
    float frac = m - (float)ip;

    char fd[40];
    if (prec > 39) prec = 39;               // clamp to the digit buffer
    for (int k = 0; k < prec; k++) {
        frac *= 10.0f;
        int d = (int)frac;                  // in [0, 10)
        if (d < 0) d = 0;
        if (d > 9) d = 9;
        fd[k] = (char)('0' + d);
        frac -= (float)d;
    }
    if (frac >= 0.5f) {                      // round the last digit up, carrying
        int k = prec - 1;
        while (k >= 0 && fd[k] == '9') { fd[k] = '0'; k--; }
        if (k >= 0) fd[k]++;
        else ip++;                          // carry rippled past all the digits
    }

    // Integer-part digits, produced low-to-high (emitted reversed below).
    char id[16];
    int nid = 0;
    if (ip == 0) id[nid++] = '0';
    else { unsigned int v = ip; while (v != 0) { id[nid++] = (char)('0' + v % 10); v /= 10; } }

    int dot = (prec > 0) || (flags & __PF_ALT);
    int content = (sign ? 1 : 0) + nid + prec + (dot ? 1 : 0);

    // '0' flag zero-pads to the width (ignored with '-').
    int zpad = 0;
    if ((flags & __PF_ZERO) && !(flags & __PF_LEFT) && width > content)
        zpad = width - content;
    int spad = (width > content + zpad) ? width - content - zpad : 0;

    if (!(flags & __PF_LEFT)) __pf_pad(o, ' ', spad);
    if (sign) __pf_put(o, sign);
    __pf_pad(o, '0', zpad);
    for (int i = nid; i > 0; i--) __pf_put(o, id[i - 1]);
    if (dot) __pf_put(o, '.');
    for (int k = 0; k < prec; k++) __pf_put(o, fd[k]);
    if (flags & __PF_LEFT) __pf_pad(o, ' ', spad);
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
        while (fmt[i] == 'h' || fmt[i] == 'z' || fmt[i] == 't' || fmt[i] == 'j') i++;

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
            case 'F': {
                // The float vararg reached us promoted to `double`, but uvclang
                // carries it as its low-32 f32 bits in the 8-byte slot. Read that
                // slot as a plain integer (a GP `va_arg`, so it stays in step
                // with the other conversions) and bit-reinterpret the low word as
                // a float. UVM has no f64, so this is single precision only.
                unsigned long bits = va_arg(ap, unsigned long);
                union { unsigned int u; float f; } bc;
                bc.u = (unsigned int)bits;
                __pf_emit_float(o, bc.f, width, prec, flags, c == 'F');
                break;
            }
            default:
                // Unknown/unsupported specifier (e.g. %f): echo it verbatim and
                // do NOT consume a vararg — we couldn't interpret its slot.
                __pf_put(o, '%');
                __pf_put(o, c);
                break;
        }
    }
}

int vprintf(const char *fmt, va_list ap)
{
    __pf_sink o = { 0, 0, 0 };
    __pf_format(&o, fmt, ap);
    return (int)o.len;
}

int printf(const char *fmt, ...)
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
int vsnprintf(char *buf, unsigned long size, const char *fmt, va_list ap)
{
    __pf_sink o = { buf, size, 0 };
    __pf_format(&o, fmt, ap);
    if (size > 0)
        buf[o.len < size ? o.len : size - 1] = '\0';
    return (int)o.len;
}

int snprintf(char *buf, unsigned long size, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return n;
}

// Unbounded sprintf: a snprintf whose buffer is treated as effectively infinite.
int vsprintf(char *buf, const char *fmt, va_list ap)
{
    return vsnprintf(buf, (unsigned long)-1, fmt, ap);
}

int sprintf(char *buf, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, (unsigned long)-1, fmt, ap);
    va_end(ap);
    return n;
}

#endif // __STDIO_H__
