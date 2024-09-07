#include <uvm/syscalls.h>
#include <stdio.h>

#define NUM_THREADS 100
#define NUM_INCRS 100

u64 thread_ids[NUM_THREADS];

u64 locked = 0;

u64 counter = 0;

void lock()
{
    for (;;)
    {
        // Try to acquire the lock
        u64 val = asm (&locked, 0, 1) -> u64 { atomic_cas_u64; };

        // If we got the lock, stop
        if (val == 0)
            break;
    }
}

void unlock()
{
    asm (&locked, 0) -> void { atomic_store_u64; };
}

void thread_fn()
{
    for (int i = 0; i < NUM_INCRS; ++i)
    {
        lock();
        ++counter;
        unlock();
    }
}

int main()
{
    for (int i = 0; i < NUM_THREADS; ++i)
    {
        thread_ids[i] = thread_spawn(thread_fn, NULL);
    }

    for (int i = 0; i < NUM_THREADS; ++i)
    {
        thread_join(thread_ids[i]);
    }

    // Check that the counter value is correct
    assert(counter == NUM_THREADS * NUM_INCRS);
    printf("counter = %d\n", counter);

    return 0;
}
