// Exercises the LLVM intrinsics uvclang lowers inline (Phase 7):
//   llvm.abs / smax / smin / umax / umin / scmp / usub.sat / bitreverse
//   llvm.memcpy / llvm.memset (mapped to UVM syscalls)
//   llvm.lifetime.* (dropped)
// Inputs are derived from a volatile seed so -O2 can't const-fold main, and
// the result is folded into one exit code compared against native.

static int three_way(int a, int b) { return (a > b) - (a < b); }   // -> llvm.scmp
static unsigned usubsat(unsigned a, unsigned b) { return a > b ? a - b : 0; } // -> llvm.usub.sat

int main()
{
    volatile int seed = 7;
    int x = seed - 20;          // -13
    int y = seed + 3;           // 10
    unsigned u = (unsigned)seed; // 7

    int r = 0;
    r += __builtin_abs(x);                       // 13
    r += (x < y ? x : y);                        // smin -> -13
    r += (x > y ? x : y);                        // smax ->  10
    r += (int)(u < (unsigned)y ? u : (unsigned)y); // umin -> 7
    r += (int)(u > (unsigned)y ? u : (unsigned)y); // umax -> 10
    r += three_way(x, y);                        // scmp -> -1
    r += three_way(y, x);                        //         +1
    r += (int)usubsat(u, (unsigned)y);           // usub.sat(7,10) -> 0
    r += (int)usubsat((unsigned)y, u);           //         (10,7) -> 3
    r += __builtin_bitreverse8((unsigned char)(seed * 3)); // reverse bits of 21

    // memcpy / memset (+ lifetime scoping of the local buffers)
    unsigned char buf[8];
    __builtin_memset(buf, 0, sizeof buf);
    buf[3] = (unsigned char)seed;                // 7
    unsigned char cp[8];
    __builtin_memcpy(cp, buf, sizeof buf);
    for (int i = 0; i < 8; i++) r += cp[i];      // += 7

    // ctpop / ctlz / cttz. The clz/ctz inputs are kept nonzero — __builtin_clz
    // and __builtin_ctz of 0 are undefined in C — while ctpop is defined at 0.
    unsigned bc = (unsigned)(seed * 3) << 4;     // 0x150, bits 4/6/8
    r += __builtin_popcount(bc);                 // ctpop.i32 -> 3
    r += __builtin_clz(bc);                      // ctlz.i32  -> 23
    r += __builtin_ctz(bc);                      // cttz.i32  -> 4
    r += __builtin_popcount(0u);                 // ctpop.i32(0) -> 0 (defined)
    unsigned long long bl = ((unsigned long long)seed << 45)
                          | (unsigned)(seed + 2); // bits 45/46/47 and 0/3
    r += (int)__builtin_popcountll(bl);          // ctpop.i64 -> 5
    r += __builtin_clzll(bl);                    // ctlz.i64  -> 16
    r += __builtin_ctzll(bl);                    // cttz.i64  -> 0

    // Everything above sums to 205 (bitreverse8(21) = 168 dominates); the
    // bit-count block adds 3 +23 +4 +0 +5 +16 +0 = 51. Total 256 -> exit 0.
    // (The exact value is documentation only; the harness compares this exit
    // code against native, which computes the same sum.)
    return r;
}
