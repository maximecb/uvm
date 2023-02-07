#ifndef __STRING_H__
#define __STRING_H__

// TODO:
/*
#ifdef memcpy
//#unset memcpy
#define memcpy()
#endif
*/

// void memcpy(u8* dst, const u8* src, u64 num_bytes)
// Copy a block of memory in the heap from a source address to a destination address.
//#define memcpy(dst, src, num_bytes) asm (dst, src, num_bytes) -> void { syscall memcpy; }

// void memset(u8* dst, u8 value, u64 num_bytes)
// Fill a block of bytes in the heap with a given value.
//#define memset(dst, value, num_bytes) asm (dst, value, num_bytes) -> void { syscall memset; }

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

#endif
