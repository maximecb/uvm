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

// TODO:
// char* strncpy(char* destination, const char* source, size_t num)

// TODO:
// char* strncat(char* destination, const char* source, size_t num)

#endif // #ifndef __STRING_H__
