// lz77.c — a self-contained LZSS (LZ77-family) compressor/decompressor used as
// a UVM benchmark. It generates a deterministic, semi-compressible input, then
// repeatedly compresses it, decompresses the result, and verifies the round
// trip byte-for-byte. The final line prints the compressed size and a checksum
// so that a native run and a UVM run can be confirmed to agree.
//
// The codec is classic LZSS with a 4 KiB sliding window:
//   - Tokens are grouped in eights behind a control byte; bit i of the control
//     byte says whether token i is a match (1) or a literal (0).
//   - A literal is one raw byte.
//   - A match is two bytes: a 12-bit distance (1..4096, stored as dist-1) and a
//     4-bit length (3..18, stored as len-3), packed big-endian so the format is
//     endianness-independent.
// Matches are found with a bounded hash-chain search (hash of 3 bytes -> most
// recent positions), the same structure zlib's deflate uses, which keeps the
// runtime predictable instead of O(n * window).
//
// This file is deliberately plain, standard C (only <stdint.h>, <stdio.h>,
// <stdlib.h>, <string.h>) so it builds identically with the host libc (native
// reference) and with uvclang's freestanding headers (the UVM build).
//
// Build and run:
//   native:  cc -O2 lz77.c -o lz77 && ./lz77
//   UVM:     uvclang -O2 examples/lz77.c -o lz77.asm && uvm lz77.asm
//   bench:   ./bench_lz77.sh

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//-----------------------------------------------------------------------------
// Tunable parameters
//-----------------------------------------------------------------------------

// Size of the generated input buffer, in bytes.
#define INPUT_SIZE (256 * 1024)

// How many compress+decompress+verify rounds to run. Chosen so the whole
// program runs for a couple of seconds under the UVM interpreter; the native
// builds finish far quicker. The input is perturbed slightly each round so the
// optimizer cannot hoist the identical work out of the loop.
#define ITERS 30

// Sliding-window / match parameters. A 12-bit distance and 4-bit length are
// what the two-byte match encoding can represent.
#define WINDOW_SIZE 4096
#define WMASK       (WINDOW_SIZE - 1)
#define MIN_MATCH   3
#define MAX_MATCH   18            // MIN_MATCH + 15 (4-bit length field)

// Hash-chain match finder. HASH_SIZE heads, and at most MAX_CHAIN probes down a
// chain per position so the search stays bounded.
#define HASH_BITS 15
#define HASH_SIZE (1 << HASH_BITS)
#define MAX_CHAIN 128

//-----------------------------------------------------------------------------
// Working buffers (allocated once in main)
//-----------------------------------------------------------------------------

static uint8_t *g_input;    // INPUT_SIZE bytes of source data
static uint8_t *g_comp;     // compressed output (worst case ~9/8 * INPUT_SIZE)
static uint8_t *g_decomp;   // decompressed output, compared back to g_input

static int *g_head;         // HASH_SIZE chain heads (-1 == empty)
static int *g_prev;         // WINDOW_SIZE chain links, indexed by pos & WMASK

//-----------------------------------------------------------------------------
// Deterministic PRNG and input generation
//-----------------------------------------------------------------------------

// xorshift64: a small, fully deterministic generator so the input (and hence
// the checksum) is identical on every platform. We do NOT use rand(), whose
// sequence differs between the host libc and uvclang's <stdlib.h>.
static uint64_t xorshift64(uint64_t *state)
{
    uint64_t x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;
    return x;
}

// Fill `buf` with `n` bytes of semi-compressible data: mostly short copies of
// earlier bytes (which the codec turns into matches), interleaved with fresh
// literals drawn from a small alphabet. This gives a realistic mix of matches
// and literals rather than trivially compressible runs.
static void gen_input(uint8_t *buf, int n)
{
    uint64_t st = 0x0123456789abcdefULL;
    int i = 0;
    while (i < n)
    {
        uint32_t r = (uint32_t)xorshift64(&st);
        if ((r & 7) < 5 && i > 64)
        {
            // Copy a run of a few bytes from somewhere in the recent past.
            int span = (i < 2048) ? i : 2048;
            int dist = 1 + (int)(xorshift64(&st) % (uint64_t)span);
            int len  = MIN_MATCH + (int)(xorshift64(&st) % 30ULL);
            for (int k = 0; k < len && i < n; k++, i++)
                buf[i] = buf[i - dist];
        }
        else
        {
            // A fresh literal from a 64-symbol alphabet.
            buf[i++] = (uint8_t)(xorshift64(&st) & 0x3F);
        }
    }
}

//-----------------------------------------------------------------------------
// LZSS compressor
//-----------------------------------------------------------------------------

// Hash the three bytes at `p` into a chain-head index.
static uint32_t hash3(const uint8_t *p)
{
    uint32_t h = (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
    h *= 2654435761u;                 // Knuth's multiplicative hash constant
    return h >> (32 - HASH_BITS);
}

// Record that a 3-byte sequence starts at `pos` (push onto its hash chain).
static void insert_pos(const uint8_t *buf, int pos)
{
    uint32_t h = hash3(buf + pos);
    g_prev[pos & WMASK] = g_head[h];
    g_head[h] = pos;
}

// Find the longest match for the bytes at `pos` within the window, scanning at
// most MAX_CHAIN chain entries. Returns the match length (>= MIN_MATCH) and
// writes its distance to *best_dist, or 0 if no usable match was found.
static int longest_match(const uint8_t *buf, int pos, int limit, int *best_dist)
{
    int max_len = limit - pos;
    if (max_len > MAX_MATCH)
        max_len = MAX_MATCH;
    if (max_len < MIN_MATCH)
        return 0;

    uint32_t h = hash3(buf + pos);
    int cur = g_head[h];
    int best_len = MIN_MATCH - 1;     // must strictly beat this to be accepted
    int chain = MAX_CHAIN;

    while (cur >= 0 && (pos - cur) <= WINDOW_SIZE && chain-- > 0)
    {
        int len = 0;
        while (len < max_len && buf[cur + len] == buf[pos + len])
            len++;
        if (len > best_len)
        {
            best_len = len;
            *best_dist = pos - cur;
            if (len >= max_len)
                break;                // can't do better than the cap
        }
        cur = g_prev[cur & WMASK];
    }

    return (best_len >= MIN_MATCH) ? best_len : 0;
}

// Compress `n` bytes of `in` into `out`, returning the compressed byte count.
static int compress(const uint8_t *in, int n, uint8_t *out)
{
    for (int i = 0; i < HASH_SIZE; i++)
        g_head[i] = -1;

    int op = 0;              // output cursor
    int pos = 0;             // input cursor
    int ntok = 0;            // token index (for control-byte bit position)
    int ctrl_idx = 0;        // output offset of the current control byte

    while (pos < n)
    {
        // Every 8 tokens, emit a fresh (zeroed) control byte.
        if ((ntok & 7) == 0)
        {
            ctrl_idx = op;
            out[op++] = 0;
        }

        int dist = 0;
        int len = 0;
        if (pos + MIN_MATCH <= n)
            len = longest_match(in, pos, n, &dist);

        if (len >= MIN_MATCH)
        {
            out[ctrl_idx] |= (uint8_t)(1u << (ntok & 7));

            uint32_t code = ((uint32_t)(dist - 1) << 4) | (uint32_t)(len - MIN_MATCH);
            out[op++] = (uint8_t)(code >> 8);
            out[op++] = (uint8_t)(code & 0xFF);

            // Insert every position the match covers so later searches see them.
            int end = pos + len;
            while (pos < end)
            {
                if (pos + MIN_MATCH <= n)
                    insert_pos(in, pos);
                pos++;
            }
        }
        else
        {
            out[op++] = in[pos];
            if (pos + MIN_MATCH <= n)
                insert_pos(in, pos);
            pos++;
        }

        ntok++;
    }

    return op;
}

//-----------------------------------------------------------------------------
// LZSS decompressor
//-----------------------------------------------------------------------------

// Decompress `n_in` bytes of `in` into `out`, returning the byte count written.
static int decompress(const uint8_t *in, int n_in, uint8_t *out)
{
    int ip = 0;
    int op = 0;
    uint8_t ctrl = 0;
    int ctrl_bits = 0;

    while (ip < n_in)
    {
        if (ctrl_bits == 0)
        {
            ctrl = in[ip++];
            ctrl_bits = 8;
        }

        int is_match = ctrl & 1;
        ctrl >>= 1;
        ctrl_bits--;

        if (is_match)
        {
            uint32_t hi = in[ip++];
            uint32_t lo = in[ip++];
            uint32_t code = (hi << 8) | lo;
            int dist = (int)(code >> 4) + 1;
            int len  = (int)(code & 0xF) + MIN_MATCH;

            int src = op - dist;
            // Byte-by-byte copy: overlapping copies (src close to op) are the
            // normal LZ case and must read bytes as they are produced.
            for (int k = 0; k < len; k++)
                out[op + k] = out[src + k];
            op += len;
        }
        else
        {
            out[op++] = in[ip++];
        }
    }

    return op;
}

//-----------------------------------------------------------------------------
// Checksum and driver
//-----------------------------------------------------------------------------

// FNV-1a over a byte range: an order- and content-sensitive checksum, computed
// identically regardless of platform endianness.
static uint64_t fnv1a(const uint8_t *data, int len)
{
    uint64_t h = 1469598103934665603ULL;
    for (int i = 0; i < len; i++)
    {
        h ^= data[i];
        h *= 1099511628211ULL;
    }
    return h;
}

int main(void)
{
    g_input  = (uint8_t *)malloc(INPUT_SIZE);
    g_comp   = (uint8_t *)malloc(INPUT_SIZE + INPUT_SIZE / 8 + 64);
    g_decomp = (uint8_t *)malloc(INPUT_SIZE);
    g_head   = (int *)malloc(HASH_SIZE * sizeof(int));
    g_prev   = (int *)malloc(WINDOW_SIZE * sizeof(int));

    if (!g_input || !g_comp || !g_decomp || !g_head || !g_prev)
    {
        printf("lz77: allocation failed\n");
        return 1;
    }

    gen_input(g_input, INPUT_SIZE);

    uint64_t checksum = 0;
    int comp_len = 0;

    for (int iter = 0; iter < ITERS; iter++)
    {
        // Perturb one byte per round so the compiler cannot prove the work is
        // redundant and hoist it out of the loop. The change is cumulative and
        // fully deterministic, so the checksum still matches across platforms.
        g_input[(iter * 2654435761u) % INPUT_SIZE] ^= (uint8_t)(iter + 1);

        comp_len = compress(g_input, INPUT_SIZE, g_comp);

        int dlen = decompress(g_comp, comp_len, g_decomp);
        if (dlen != INPUT_SIZE || memcmp(g_decomp, g_input, INPUT_SIZE) != 0)
        {
            printf("lz77: VERIFY FAILED on iter %d (dlen=%d)\n", iter, dlen);
            return 1;
        }

        // Fold this round's compressed data into the running checksum.
        checksum ^= fnv1a(g_comp, comp_len);
        checksum = (checksum << 1) | (checksum >> 63);
    }

    int pct = (int)((int64_t)comp_len * 100 / INPUT_SIZE);
    printf("lz77: input=%d bytes, compressed=%d bytes (%d%% of input), iters=%d\n",
           INPUT_SIZE, comp_len, pct, ITERS);
    printf("lz77: checksum=0x%016lx\n", (unsigned long)checksum);

    free(g_input);
    free(g_comp);
    free(g_decomp);
    free(g_head);
    free(g_prev);
    return 0;
}
