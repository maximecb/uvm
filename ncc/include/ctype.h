#ifndef __CTYPE_H__
#define __CTYPE_H__

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

int toupper(int ch)
{
    if (islower(ch))
    {
        return ch + ('A' - 'a');
    }

    return ch;
}

#endif
