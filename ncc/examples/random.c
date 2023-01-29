// The seed must be an odd number
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

    // Use the upper 32 bits of the state only
    return cur_rand >> 32;
}

void srand(u32 seed)
{
    // Seed must be an odd number
    cur_rand = (seed << 3) + 1;
}

void main()
{
    srand(9000);

    u32 N = 50;

    u32 min_val = 0xFFFF;
    u32 max_val = 0;
    u32 sum = 0;

    for (int i = 0; i < N; ++i)
    {
        u32 r = rand() % 10;
        sum = sum + r;

        print_i64(r);
        print_endl();
    }

    u32 avg = sum / N;
    print_str("avg: ");
    print_i64(avg);
    print_endl();
}

// Print an i64 value to standard output
inline void print_i64(i64 val)
{
    return asm (val) -> void { syscall 5; };
}

// Print a string to standard output
inline void print_str(char* str)
{
    return asm (str) -> void { syscall 6; };
}

// Print a newline to standard output
inline void print_endl()
{
    return asm () -> void { syscall 7; };
}
