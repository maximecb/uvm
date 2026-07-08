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
    // Take the magnitude using the sign bit captured above rather than a
    // `val < 0.0f` compare: newer clang (>=21) proves `(float)pun.d < 0.0f`
    // equals an f64 compare on the un-narrowed double and hoists it to an
    // `fcmp olt double`, which UVM has no instruction for. Branching on `neg`
    // keeps the whole routine in f32.
    if (neg) val = -val;               // magnitude; sign captured above

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
