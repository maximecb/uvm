// Self-checking test for the <stdlib.h> free-list allocator: exercises block
// reuse, coalescing, calloc zeroing, realloc grow/shrink, and thread-safe
// concurrent malloc/free. Asserts its own results; exit 0 means all passed.
// (uvm_ prefix -> the harness runs it standalone, no native reference.)
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <stdint.h>

#define NUM_THREADS 4
#define ITERS 300

// Each worker churns its own allocations, writing a per-block byte pattern and
// verifying it just before freeing. If the lock failed to serialize the heap,
// two threads would be handed the same chunk and a pattern check would fail.
static void *worker(void *arg)
{
    int id = (int)(intptr_t)arg;
    for (int iter = 0; iter < ITERS; iter++)
    {
        void *p[16];
        for (int i = 0; i < 16; i++)
        {
            size_t sz = (size_t)((i * 13 + iter + id) % 200 + 1);
            p[i] = malloc(sz);
            assert(p[i] != NULL);
            __uvm_memset((uint8_t *)p[i], (uint8_t)(id * 16 + i), sz);
        }
        for (int i = 0; i < 16; i++)
        {
            size_t sz = (size_t)((i * 13 + iter + id) % 200 + 1);
            uint8_t want = (uint8_t)(id * 16 + i);
            uint8_t *q = (uint8_t *)p[i];
            for (size_t k = 0; k < sz; k++)
                assert(q[k] == want);        // nobody else scribbled on us
            free(p[i]);
        }
    }
    return 0;
}

int main()
{
    // --- reuse: a freed block of the same size is handed back --------------
    void *a = malloc(64);
    assert(a != NULL);
    free(a);
    void *b = malloc(64);
    assert(b == a);                  // exact reuse of the freed chunk
    free(b);

    // --- coalescing: three adjacent chunks freed then merged ---------------
    void *c0 = malloc(48);
    void *c1 = malloc(48);
    void *c2 = malloc(48);
    assert(c0 && c1 && c2);
    free(c1);
    free(c0);                        // coalesces forward into c1
    free(c2);                        // coalesces the whole run (and into top)
    void *big = malloc(48 * 3);      // must fit in the merged space
    assert(big == c0);               // starts at the lowest freed address
    free(big);

    // --- calloc: zero-initialized and overflow-safe ------------------------
    int *z = (int *)calloc(16, sizeof(int));
    assert(z != NULL);
    for (int i = 0; i < 16; i++)
        assert(z[i] == 0);
    free(z);
    assert(calloc((size_t)-1, (size_t)-1) == NULL);   // overflow -> NULL

    // --- realloc: grow preserves contents, shrink keeps them ---------------
    char *s = (char *)malloc(8);
    __uvm_memcpy((uint8_t *)s, (const uint8_t *)"abcdefg", 8);  // 7 chars + NUL
    s = (char *)realloc(s, 256);     // grow (likely relocates)
    assert(s != NULL);
    assert(strcmp(s, "abcdefg") == 0);
    s = (char *)realloc(s, 4);       // shrink in place
    assert(s[0] == 'a' && s[1] == 'b' && s[2] == 'c');
    free(s);
    assert(realloc(NULL, 32) != NULL);          // realloc(NULL, n) == malloc

    // --- concurrency: four threads hammering malloc/free in parallel -------
    pthread_t tids[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_create(&tids[i], 0, worker, (void *)(intptr_t)i);
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(tids[i], 0);

    return 0;
}
