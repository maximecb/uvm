u64 cur_rand = 1337;

// Multiplicative Congruential Generator (MCG)
// Constant from "Computationally Easy, Spectrally Good Multipliers for
// Congruential Pseudorandom Number Generators" by Steele & Vigna.
//
//  xn = (a * xn−1) mod 2^64
//  a = 0xf1357aea2e62a9c5
//  64 output bits
//
// TODO: change return to signed i32 like in C
//
u32 rand()
{
    cur_rand = (0xf1357aea2e62a9c5 * cur_rand);
    return cur_rand & 0xFFFFFFFF;
}

void srand(u32 seed)
{
    //cur_rand = seed;
}

void main()
{
    for (u32 i = 0; i < 30; ++i)
    {
        u32 r = rand() % 10;

        //print_i64(r);
        print_endl();
    }
}

// Print an i64 value to standard output
inline void print_i64(i64 val)
{
    return asm (val) -> void { syscall 5; };
}

// Print a newline to standard output
inline void print_endl()
{
    return asm () -> void { syscall 7; };
}
