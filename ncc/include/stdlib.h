#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <stddef.h>
#include <assert.h>

int abs(int n)
{
    if (n < 0)
        return -n;
    return n;
}

void exit(int status)
{
    asm (status) -> void { exit; };
}

// Convert long int to string
char* ltoa(long value, char* str, int base)
{
    assert(base > 0 && base <= 16);

    // If negative, write a minus sign
    if (value < 0)
    {
        *str = '-';
        ++str;
        value = -value;
    }

    // Compute the number of digits
    int num_digits = 0;
    long n = value;
    do
    {
        n = n / base;
        ++num_digits;
    } while (n > 0);

    // The digits have to be written in reverse order
    for (int i = num_digits - 1; i >= 0; --i)
    {
        long digit = value % base;
        value = value / base;

        char ch;
        if (digit < 10)
        {
            ch = '0' + digit;
        }
        else
        {
            ch = 'A' + (digit - 10);
        }

        str[i] = ch;
    }

    // Write the null terminator
    str[num_digits] = '\0';
}

// Convert int to string
char* itoa(int value, char* str, int base)
{
    return ltoa((long)value, str, base);
}

// We define RAND_MAX to be the same as INT32_MAX
#define RAND_MAX 0x7FFFFFFF

// The state has 64 bits
// The seed must be an odd number
unsigned long __cur_rand__ = 1337;

// Multiplicative Congruential Generator (MCG)
// Constant from "Computationally Easy, Spectrally Good Multipliers for
// Congruential Pseudorandom Number Generators" by Steele & Vigna.
//
//  xn = (a * xn−1) mod 2^64
//  a = 0xf1357aea2e62a9c5
//  64 output bits
//
int rand()
{
    __cur_rand__ = (0xf1357aea2e62a9c5 * __cur_rand__);

    // Use the upper 31 bits of the state only
    return (int)(__cur_rand__ >> 33);
}

void srand(unsigned int seed)
{
    // Seed must be an odd number,
    // So we make sure the lowest bit is 1
    __cur_rand__ = (seed << 1) + 1;
}

#define align_ptr(ptr, n_bytes) (((u64)(ptr) + ((n_bytes) - 1)) & ~((n_bytes) - 1))

u64 __heap_size__ = 0;
u8* __next_alloc__ = 0;

void* malloc(size_t size)
{
    // If this is the first allocation
    if (__next_alloc__ == 0)
    {
        __heap_size__ = asm () -> u64 { syscall vm_heap_size; };
        __next_alloc__ = (u8*)__heap_size__;
    }

    // Bump the allocation pointer
    u8* header_ptr = __next_alloc__;
    u8* block_ptr = header_ptr + 8;
    __next_alloc__ = align_ptr(block_ptr + size, 8);

    // Resize the heap if needed
    if (__next_alloc__ > __heap_size__)
    {
        __heap_size__ = asm (__next_alloc__) -> u64 { syscall vm_grow_heap; };
    }

    // Write a magic word at the beginning of the block for safety checks
    u32* magic_ptr = (u32*)header_ptr;
    *magic_ptr = 0x1337BAB3;

    return (void*)block_ptr;
}

void free(void* ptr)
{
    // Verify and clear the magic word
    // This will help detect double-free errors
    u8* header_ptr = ((u8*)ptr) - 8;
    u32* magic_ptr = (u32*)header_ptr;

    if (*magic_ptr != 0x1337BAB3)
    {
        asm ("magic word does not match in free()\n") -> void {
            syscall print_str;
            panic;
        };
    }

    // Corrupt the magic word to detect double-free errors
    *magic_ptr = 0x1111_1111;
}

#endif
