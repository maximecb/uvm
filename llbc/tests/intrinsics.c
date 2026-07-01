// Exercises the LLVM intrinsics llbc lowers inline (Phase 7):
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

    // r = 13 -13 +10 +7 +10 -1 +1 +0 +3 +240 +7 = 277 -> exit 277 & 0xFF = 21
    return r;
}
