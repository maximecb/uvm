#ifndef __STDLIB_H__
#define __STDLIB_H__

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

    // Use the upper 32 bits of the state only
    return (int)(__cur_rand__ >> 32);
}

void srand(unsigned int seed)
{
    // Seed must be an odd number,
    // So we make sure the lowest bit is 1
    __cur_rand__ = (seed << 1) + 1;
}

#define align_ptr(ptr, n_bytes) (((u64)(ptr) + ((n_bytes) - 1)) & ~((n_bytes) - 1))

void* malloc(size_t size)
{
    u64 heap_size = asm () -> u64 { syscall vm_heap_size; };

    u64 header_ptr = align_ptr(heap_size, 8);
    u64 block_ptr = header_ptr + 8;

    // Resize the heap
    u64 new_heap_size = block_ptr + size;
    asm (new_heap_size) -> void { syscall vm_resize_heap; };

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
        asm () -> void { panic; };
    }

    *magic_ptr != 0x1111_1111;
}

#endif
