// Self-checking test for the pthreads-style mutex in <pthread.h>.
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

int main()
{
    int rc = check_trylock();
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
