#ifndef __CTYPE_H__
#define __CTYPE_H__

int isprint(int c)
{
    return (c >= 0x20 && c <= 0x7E);
}

int islower(int c)
{
    return (c >= 'a' && c <= 'z');
}

int isupper(int c)
{
    return (c >= 'A' && c <= 'Z');
}

#endif
