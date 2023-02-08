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

// TODO:
//void* malloc(size_t size)

// TODO:
//void free(void* ptr)

#endif
