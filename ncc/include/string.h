#ifndef __STRING_H__
#define __STRING_H__

#ifndef memcpy
#define memcpy(dst, src, num_bytes) asm (dst, src, num_bytes) -> void { syscall memcpy; }
#endif

#ifndef memset
#define memset(dst, value, num_bytes) asm (dst, value, num_bytes) -> void { syscall memset; }
#endif

size_t strlen(char* p)
{
    size_t l = 0;
    while (*(p + l) != 0) l = l + 1;
    return l;
}

int strcmp(char* a, char* b)
{
    for (size_t i = 0;; ++i)
    {
        char ch_a = a[i];
        char ch_b = b[i];

        if (ch_a < ch_b)
            return -1;
        else if (ch_a > ch_b)
            return 1;

        if (ch_a == 0)
            break;
    }

    return 0;
}

int strncmp(char* a, char* b, size_t num)
{
    for (size_t i = 0; i < num; ++i)
    {
        char ch_a = a[i];
        char ch_b = b[i];

        if (ch_a < ch_b)
            return -1;
        else if (ch_a > ch_b)
            return 1;

        if (ch_a == 0)
            break;
    }

    return 0;
}

// char* strchr(const char *str, int c)
char* strchr(char *str, int c)
{
    while (*str)
    {
        if (*str == c)
            return str;

        ++str;
    }

    return NULL;
}

// Returns a pointer to the first occurrence of str2 in str1,
// or a null pointer if str2 is not part of str1.
char* strstr(char* s1, char* s2)
{
    char* p1 = s1;
    char* p2;

    while (*s1)
    {
        p2 = s2;

        while (*p2 && (*p1 == *p2))
        {
            ++p1;
            ++p2;
        }

        if (!*p2)
        {
            return (char*)s1;
        }

        ++s1;
        p1 = s1;
    }

    return NULL;
}

// TODO:
// char* strncpy(char* destination, const char* source, size_t num)

// TODO:
// char* strncat(char* destination, const char* source, size_t num)

#endif // #ifndef __STRING_H__
