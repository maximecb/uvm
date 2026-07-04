#ifndef __CTYPE_H__
#define __CTYPE_H__

// <ctype.h> for the uvclang/UVM freestanding target. These are ordinary C
// definitions (ASCII only) that uvclang compiles like any other function. This
// header is used ONLY for the UVM build (clang with -Iuvclang/include); the
// native reference build uses the platform's libc.

// Every definition below carries weak linkage (UVCLANG_WEAK). That lets this
// header be #included from any number of translation units without producing
// duplicate-symbol errors when they are linked together (LLVM keeps a single
// copy of each). It needs no per-file "implementation" opt-in, and in a
// single-translation-unit build (all uvclang builds today) it has no effect --
// uvclang parses and ignores the linkage attribute.
#ifndef UVCLANG_WEAK
#define UVCLANG_WEAK __attribute__((weak))
#endif

UVCLANG_WEAK int isalnum(int c)
{
    return (
        (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9')
    );
}

// Check if c is a printable character
// Note that this includes spaces, but excludes \t, \r and \n
UVCLANG_WEAK int isprint(int c)
{
    return (c >= 0x20 && c <= 0x7E);
}

UVCLANG_WEAK int isspace(int c)
{
    return (
        c == '\t'   || // 0x09
        c == '\n'   || // 0x0a
        c == 0x0b   || // vertical tab, \v
        c == '\r'   || // 0x0d
        c == 0x0c   || // form feed, \f
        c == ' '       // 0x20
    );
}

UVCLANG_WEAK int islower(int c)
{
    return (c >= 'a' && c <= 'z');
}

UVCLANG_WEAK int isupper(int c)
{
    return (c >= 'A' && c <= 'Z');
}

UVCLANG_WEAK int isdigit(int c)
{
    return (c >= '0' && c <= '9');
}

UVCLANG_WEAK int isalpha(int c)
{
    return (
        (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z')
    );
}

// Control characters: 0x00-0x1F and DEL (0x7F).
UVCLANG_WEAK int iscntrl(int c)
{
    return (c >= 0x00 && c <= 0x1F) || c == 0x7F;
}

// Any printable character except space (i.e. isprint minus 0x20).
UVCLANG_WEAK int isgraph(int c)
{
    return (c >= 0x21 && c <= 0x7E);
}

UVCLANG_WEAK int isxdigit(int c)
{
    return (
        (c >= '0' && c <= '9') ||
        (c >= 'a' && c <= 'f') ||
        (c >= 'A' && c <= 'F')
    );
}

// Blank characters used to separate words: space and horizontal tab.
UVCLANG_WEAK int isblank(int c)
{
    return (c == ' ' || c == '\t');
}

// Printable, but neither a space nor alphanumeric.
UVCLANG_WEAK int ispunct(int c)
{
    return isgraph(c) && !isalnum(c);
}

UVCLANG_WEAK int tolower(int ch)
{
    if (isupper(ch))
    {
        return ch + ('a' - 'A');
    }

    return ch;
}

UVCLANG_WEAK int toupper(int ch)
{
    if (islower(ch))
    {
        return ch + ('A' - 'a');
    }

    return ch;
}

#endif
