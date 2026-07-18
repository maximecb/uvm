// Self-checking test for <pthread.h>: the pthreads-style mutex, plus thread
// creation with a configurable private stack size (pthread_attr_t).
//
// The counter here is a PLAIN (non-atomic) uint64_t: the only thing making the
// concurrent increments safe is the mutex. If pthread_mutex_lock/unlock did not
// provide real mutual exclusion, the read-modify-write `shared++` would lose
// updates under contention and the final total would fall short. Landing
// exactly on NUM_THREADS * ITERS is the proof the lock works.

#include <stdint.h>
#include <pthread.h>

#define NUM_THREADS 4
#define ITERS 5000

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static uint64_t shared; // deliberately not _Atomic — protected by `lock`

static void* worker(void* arg)
{
    (void)arg;
    for (int i = 0; i < ITERS; i++) {
        pthread_mutex_lock(&lock);
        shared++;
        pthread_mutex_unlock(&lock);
    }
    return 0;
}

// trylock should fail on an already-held mutex and succeed once it is released.
static int check_trylock(void)
{
    pthread_mutex_t m;
    pthread_mutex_init(&m, 0);

    if (pthread_mutex_lock(&m) != 0) return 30;
    if (pthread_mutex_trylock(&m) != EBUSY) return 31; // held -> busy
    if (pthread_mutex_unlock(&m) != 0) return 32;
    if (pthread_mutex_trylock(&m) != 0) return 33;     // free -> acquired
    if (pthread_mutex_unlock(&m) != 0) return 34;

    pthread_mutex_destroy(&m);
    return 0;
}

// The attribute API: a fresh attr reports the default size; a set within range
// round-trips; a set below PTHREAD_STACK_MIN is rejected without mutating it.
static int check_attr(void)
{
    pthread_attr_t a;
    size_t s;

    if (pthread_attr_init(&a) != 0) return 40;
    if (pthread_attr_getstacksize(&a, &s) != 0) return 41;
    if (s != PTHREAD_STACK_SIZE) return 42;                    // default after init

    if (pthread_attr_setstacksize(&a, 1024 * 1024) != 0) return 43;
    if (pthread_attr_getstacksize(&a, &s) != 0) return 44;
    if (s != 1024 * 1024) return 45;                           // round-trips

    if (pthread_attr_setstacksize(&a, 128) != EINVAL) return 46; // below MIN -> rejected
    if (pthread_attr_getstacksize(&a, &s) != 0) return 47;
    if (s != 1024 * 1024) return 48;                           // unchanged after EINVAL

    pthread_attr_destroy(&a);
    return 0;
}

// Recursion whose every frame holds an address-taken (volatile) buffer, forcing
// each call to consume the thread's private alloca stack rather than living in
// SSA registers. deep_sum(n) = 7 + sum_{k=1..n} k = 7 + n*(n+1)/2.
static uint64_t deep_sum(int depth)
{
    volatile uint64_t buf[8];
    buf[0] = (uint64_t)depth;
    for (int i = 1; i < 8; i++) buf[i] = buf[i - 1] + 1;
    if (depth == 0) return buf[7];
    return buf[0] + deep_sum(depth - 1);
}

#define STACK_DEPTH 3000

static void* stack_worker(void* arg)
{
    int depth = (int)(intptr_t)arg;
    return (void*)(uintptr_t)deep_sum(depth);
}

// Run a recursion deep enough to need well over the default stack, on a thread
// created with a 1 MiB private stack, and check it returns the exact sum. This
// exercises pthread_create honoring pthread_attr_t's stack size end to end.
static int check_custom_stack(void)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (pthread_attr_setstacksize(&attr, 1024 * 1024) != 0) return 50;

    pthread_t t;
    if (pthread_create(&t, &attr, stack_worker, (void*)(intptr_t)STACK_DEPTH) != 0) return 51;

    void* ret;
    pthread_join(t, &ret);
    pthread_attr_destroy(&attr);

    uint64_t expected = 7 + (uint64_t)STACK_DEPTH * (STACK_DEPTH + 1) / 2;
    if ((uint64_t)(uintptr_t)ret != expected) return 52;
    return 0;
}

int main()
{
    int rc = check_trylock();
    if (rc != 0) return rc;

    rc = check_attr();
    if (rc != 0) return rc;

    rc = check_custom_stack();
    if (rc != 0) return rc;

    shared = 0;
    pthread_t tids[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_create(&tids[i], 0, worker, 0);
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(tids[i], 0);

    if (shared != (uint64_t)NUM_THREADS * ITERS) return 1;
    return 0;
}
