// Self-checking test for C11 atomics (<stdatomic.h>), which uvclang lowers to
// the UVM atomic_load_u64 / atomic_store_u64 / atomic_cas_u64 ops. Atomics are
// 64-bit only, so every atomic object here is a 64-bit type.
//
// Part 1 checks each operation's value semantics single-threaded. Part 2 runs
// several threads hammering a shared counter (via both atomicrmw and an
// explicit cmpxchg loop) and verifies no updates are lost, which only holds if
// the read-modify-writes are genuinely atomic under contention.
//
// The worker threads are created with pthread_create, not the raw thread_spawn
// syscall: raw-spawned threads share the fallback software stack, so at -O0
// (where every local, including the loop counter, is an address-taken alloca)
// concurrent workers would clobber each other's frames. pthread_create gives
// each thread a private stack, so the contention here exercises the atomics —
// not stack aliasing.

#include <stdatomic.h>
#include <stdint.h>
#include <pthread.h>

_Atomic uint64_t g;

static int single_threaded(void)
{
    atomic_store(&g, 100);
    if (atomic_load(&g) != 100) return 1;

    // fetch_* return the value held *before* the update.
    if (atomic_fetch_add(&g, 5) != 100) return 2;
    if (atomic_load(&g) != 105) return 3;
    if (atomic_fetch_sub(&g, 10) != 105) return 4;
    if (atomic_load(&g) != 95) return 5;

    atomic_store(&g, 0xF0);
    if (atomic_fetch_and(&g, 0x3C) != 0xF0) return 6;
    if (atomic_load(&g) != 0x30) return 7;
    if (atomic_fetch_or(&g, 0x0F) != 0x30) return 8;
    if (atomic_load(&g) != 0x3F) return 9;
    if (atomic_fetch_xor(&g, 0xFF) != 0x3F) return 10;
    if (atomic_load(&g) != 0xC0) return 11;

    if (atomic_exchange(&g, 777) != 0xC0) return 12;
    if (atomic_load(&g) != 777) return 13;

    // CAS success: swaps and reports true.
    uint64_t expected = 777;
    if (!atomic_compare_exchange_strong(&g, &expected, 888)) return 14;
    if (atomic_load(&g) != 888) return 15;

    // CAS failure: leaves the object untouched and writes the actual value
    // back into the expected slot.
    uint64_t wrong = 999;
    if (atomic_compare_exchange_strong(&g, &wrong, 111)) return 16;
    if (wrong != 888) return 17;
    if (atomic_load(&g) != 888) return 18;

    return 0;
}

#define NUM_THREADS 4
#define ITERS 5000

_Atomic uint64_t rmw_counter;
_Atomic uint64_t cas_counter;

static void* worker(void* arg)
{
    (void)arg;
    for (int i = 0; i < ITERS; i++) {
        // Increment one counter with a single atomicrmw...
        atomic_fetch_add(&rmw_counter, 1);

        // ...and the other with a hand-rolled cmpxchg retry loop.
        uint64_t cur = atomic_load(&cas_counter);
        while (!atomic_compare_exchange_weak(&cas_counter, &cur, cur + 1)) {
            // cur has been refreshed with the current value; try again.
        }
    }
    return 0;
}

static int multi_threaded(void)
{
    atomic_store(&rmw_counter, 0);
    atomic_store(&cas_counter, 0);

    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_create(&threads[i], 0, worker, 0);
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], 0);

    uint64_t expected = (uint64_t)NUM_THREADS * ITERS;
    if (atomic_load(&rmw_counter) != expected) return 20;
    if (atomic_load(&cas_counter) != expected) return 21;
    return 0;
}

int main()
{
    int rc = single_threaded();
    if (rc != 0) return rc;
    return multi_threaded();
}
