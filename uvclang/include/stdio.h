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
// Supported conversions: %d %i %u %x %X %o %c %s %p %% and %f/%F, with flags
// (- 0 + space #), field width and precision (both literal and `*`), and the
// `l`/`ll` (tolerated `h`/`hh`/`z`/`t`/`j`) length modifiers. The integer/string
// conversions are matched byte-for-byte against native libc by tests/printf.c.
//
// %f caveat: a `%f` argument is a `double`, but UVM has no f64 arithmetic and
// uvclang doesn't model the x86_64 XMM vararg save area. Two things make it work
// anyway: (1) uvclang packs every vararg into one linear 8-byte-slot buffer, so
// the double's raw bits sit exactly where an *integer* va_arg would read them —
// we pull them out with `va_arg(ap, unsigned long)`, keeping the walk in sync;
// (2) we narrow the double to a 32-bit float (via the UVM f64_to_f32 insn) and
// format that. So `%f` output is only float-accurate (~7 significant digits) and
// does NOT match native libc's double formatting byte-for-byte — it is tested
// tolerantly (format, parse back, compare with a tolerance) in tests/printf_float.c,
// never by exact-string comparison. Very large magnitudes (whose integer part
// exceeds 2^31) and %e/%g are not supported.
//
// Any other unsupported specifier is echoed verbatim and its argument left
// unconsumed — correct only if no further integer conversions follow it.

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

// Emit a `%f` conversion from the raw 64-bit pattern `ubits` of the `double`
// argument. UVM has no f64 arithmetic, so we detect the sign and inf/nan cases
// straight from the IEEE-754 bit fields, then narrow the finite magnitude to a
// 32-bit float (f64_to_f32) and produce digits with f32 arithmetic. The result
// is therefore only float-accurate; see the file header. `upper` selects the
// INF/NAN spelling for %F.
static void __pf_emit_float(__pf_sink *o, unsigned long ubits, int width, int prec,
                            int flags, int upper)
{
    if (prec < 0)
        prec = 6;   // C default precision for %f

    int neg = (int)(ubits >> 63);
    unsigned expo = (unsigned)((ubits >> 52) & 0x7FF);
    char sign = neg ? '-' : (flags & __PF_PLUS) ? '+' : (flags & __PF_SPACE) ? ' ' : 0;

    // inf / nan: exponent field is all ones. Mantissa distinguishes them.
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

    // Finite: reinterpret the bits as a double and narrow to a float magnitude.
    union { unsigned long u; double d; } pun;
    pun.u = ubits;
    float val = (float)pun.d;          // f64 -> f32
    if (val < 0.0f) val = -val;        // magnitude; sign captured above

    // Round half away from zero at the last printed place: bias by 0.5 ulp of
    // the precision, then truncate each digit. The bias carries into the
    // integer part on its own, so 0.9999995 -> "1.000000" falls out naturally.
    float half = 0.5f;
    for (int i = 0; i < prec; i++)
        half *= 0.1f;
    val += half;

    // Integer part (32-bit: magnitudes past 2^31 are beyond a float's exact
    // range anyway — see the header's limitation note).
    unsigned ipart = (unsigned)val;
    float frac = val - (float)ipart;

    char id[16];
    int il = 0;
    if (ipart == 0)
        id[il++] = '0';
    else
        while (ipart) { id[il++] = (char)('0' + ipart % 10); ipart /= 10; }

    int content = (sign ? 1 : 0) + il + (prec > 0 ? 1 + prec : 0);
    int zpad = 0;
    if ((flags & __PF_ZERO) && !(flags & __PF_LEFT) && width > content)
        zpad = width - content;
    int spad = (width > content + zpad) ? width - content - zpad : 0;

    if (!(flags & __PF_LEFT)) __pf_pad(o, ' ', spad);
    if (sign) __pf_put(o, sign);
    __pf_pad(o, '0', zpad);
    for (int i = il; i > 0; i--)
        __pf_put(o, id[i - 1]);
    if (prec > 0) {
        __pf_put(o, '.');
        for (int i = 0; i < prec; i++) {
            frac *= 10.0f;
            unsigned dig = (unsigned)frac;   // in [0,9]
            if (dig > 9u) dig = 9u;          // guard against f32 rounding fuzz
            __pf_put(o, (char)('0' + dig));
            frac -= (float)dig;
        }
    }
    if (flags & __PF_LEFT) __pf_pad(o, ' ', spad);
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
            case 'F':
                // A `double` arg. We can't use the XMM save area, but its 64 bits
                // sit in the linear vararg buffer where an integer va_arg reads,
                // so pull them out through the GP path (this also keeps the walk
                // in sync for any following conversions). See the file header.
                __pf_emit_float(o, va_arg(ap, unsigned long), width, prec, flags, c == 'F');
                break;
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
