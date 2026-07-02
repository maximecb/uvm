#ifndef __CTYPE_H__
#define __CTYPE_H__

// <ctype.h> for the uvclang/UVM freestanding target. These are ordinary C
// definitions (ASCII only) that uvclang compiles like any other function. Ported
// from ncc/include/ctype.h. This header is used ONLY for the UVM build (clang
// with -Iuvclang/include); the native reference build uses the platform's libc.

int isalnum(int c)
{
    return (
        (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9')
    );
}

// Check if c is a printable character
// Note that this includes spaces, but excludes \t, \r and \n
int isprint(int c)
{
    return (c >= 0x20 && c <= 0x7E);
}

int isspace(int c)
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

int islower(int c)
{
    return (c >= 'a' && c <= 'z');
}

int isupper(int c)
{
    return (c >= 'A' && c <= 'Z');
}

int isdigit(int c)
{
    return (c >= '0' && c <= '9');
}

int isalpha(int c)
{
    return (
        (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z')
    );
}

// Control characters: 0x00-0x1F and DEL (0x7F).
int iscntrl(int c)
{
    return (c >= 0x00 && c <= 0x1F) || c == 0x7F;
}

// Any printable character except space (i.e. isprint minus 0x20).
int isgraph(int c)
{
    return (c >= 0x21 && c <= 0x7E);
}

int isxdigit(int c)
{
    return (
        (c >= '0' && c <= '9') ||
        (c >= 'a' && c <= 'f') ||
        (c >= 'A' && c <= 'F')
    );
}

// Blank characters used to separate words: space and horizontal tab.
int isblank(int c)
{
    return (c == ' ' || c == '\t');
}

// Printable, but neither a space nor alphanumeric.
int ispunct(int c)
{
    return isgraph(c) && !isalnum(c);
}

int tolower(int ch)
{
    if (isupper(ch))
    {
        return ch + ('a' - 'A');
    }

    return ch;
}

int toupper(int ch)
{
    if (islower(ch))
    {
        return ch + ('A' - 'a');
    }

    return ch;
}

#endif
