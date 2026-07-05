#ifndef __PTHREAD_H__
#define __PTHREAD_H__

// A small pthreads-style API for the uvclang/UVM target: mutexes plus thin
// thread create/join wrappers. It sits on two UVM facilities:
//   * the C11 atomics (<stdatomic.h>), which uvclang lowers to the UVM
//     atomic_load_u64 / atomic_store_u64 / atomic_cas_u64 ops, and
//   * the thread syscalls (thread_spawn / thread_join / thread_sleep).
//
// This is a freestanding subset, not a POSIX-complete pthreads. The mutex is a
// spin lock: correct because guest threads are real, preemptively scheduled OS
// threads (a blocked waiter never prevents the holder from making progress),
// with a yield on contention so it does not monopolize a core. There is no
// recursive/error-checking mutex, no condition variables, and no attributes.
//
// Because the UVM atomic ops are 64 bits wide, pthread_mutex_t is 64-bit backed.
//
// Used ONLY for the UVM build (clang with -Iuvclang/include). Every definition
// carries weak linkage so the header can be #included from any number of
// translation units without duplicate-symbol errors at link time.

#include <stdint.h>          // uint64_t / uintptr_t
#include <stdatomic.h>       // _Atomic, atomic_* (lowered to UVM atomic ops)
#include <uvm/syscalls.h>    // __uvm_thread_spawn / __uvm_thread_join / __uvm_thread_sleep

#ifndef UVCLANG_WEAK
#define UVCLANG_WEAK __attribute__((weak))
#endif

// Returned by pthread_mutex_trylock when the lock is already held.
#ifndef EBUSY
#define EBUSY 16
#endif

// Mutex state values. Kept 64-bit wide to match the UVM atomic ops.
#define __PTHREAD_MUTEX_UNLOCKED 0
#define __PTHREAD_MUTEX_LOCKED   1

// A mutex is a single atomic word: unlocked (0) or locked (1). The struct
// wrapper matches the pthreads convention of passing mutexes by address and
// keeps PTHREAD_MUTEX_INITIALIZER usable as a brace initializer.
typedef struct
{
    _Atomic uint64_t __state;
} pthread_mutex_t;

// Static initializer for an unlocked mutex: pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
#define PTHREAD_MUTEX_INITIALIZER { __PTHREAD_MUTEX_UNLOCKED }

// Initialize a mutex to the unlocked state. attr is ignored (no attributes are
// supported). Always returns 0.
UVCLANG_WEAK int pthread_mutex_init(pthread_mutex_t* mutex, const void* attr)
{
    (void)attr;
    atomic_store(&mutex->__state, __PTHREAD_MUTEX_UNLOCKED);
    return 0;
}

// Destroy a mutex. Nothing to release; provided for API symmetry. Returns 0.
UVCLANG_WEAK int pthread_mutex_destroy(pthread_mutex_t* mutex)
{
    (void)mutex;
    return 0;
}

// Try to lock without blocking. Returns 0 if the lock was acquired, or EBUSY if
// it was already held.
UVCLANG_WEAK int pthread_mutex_trylock(pthread_mutex_t* mutex)
{
    uint64_t expected = __PTHREAD_MUTEX_UNLOCKED;
    if (atomic_compare_exchange_strong(&mutex->__state, &expected, __PTHREAD_MUTEX_LOCKED)) {
        return 0;
    }
    return EBUSY;
}

// Block until the mutex is acquired. Spins on a compare-and-swap, yielding the
// OS thread between failed attempts so a contended lock does not burn a core.
// The successful CAS carries acquire ordering, pairing with the release in
// pthread_mutex_unlock. Always returns 0.
UVCLANG_WEAK int pthread_mutex_lock(pthread_mutex_t* mutex)
{
    uint64_t expected = __PTHREAD_MUTEX_UNLOCKED;
    while (!atomic_compare_exchange_weak(&mutex->__state, &expected, __PTHREAD_MUTEX_LOCKED)) {
        // A failed compare_exchange overwrites `expected` with the value it
        // actually found; reset it so the next attempt again swaps 0 -> 1.
        expected = __PTHREAD_MUTEX_UNLOCKED;
        thread_sleep(0); // yield to the lock holder / other threads
    }
    return 0;
}

// Release the mutex. Stores the unlocked state with release ordering so writes
// made under the lock are visible to the next thread that acquires it. Always
// returns 0.
UVCLANG_WEAK int pthread_mutex_unlock(pthread_mutex_t* mutex)
{
    atomic_store(&mutex->__state, __PTHREAD_MUTEX_UNLOCKED);
    return 0;
}

// -- threads --------------------------------------------------------------
//
// Thin wrappers over the UVM thread syscalls so callers can use the familiar
// pthread spellings. A pthread_t is just a UVM thread id.

typedef uint64_t pthread_t;

// Start a new thread running start(arg). attr is ignored. On success writes the
// new thread id through `thread` (if non-NULL) and returns 0.
UVCLANG_WEAK int pthread_create(pthread_t* thread, const void* attr, void* (*start)(void*), void* arg)
{
    (void)attr;
    uint64_t tid = thread_spawn((void*)start, arg);
    if (thread) {
        *thread = tid;
    }
    return 0;
}

// Wait for `thread` to finish. If retval is non-NULL, the thread's return value
// is stored there. Returns 0.
UVCLANG_WEAK int pthread_join(pthread_t thread, void** retval)
{
    uint64_t r = thread_join(thread);
    if (retval) {
        *retval = (void*)(uintptr_t)r;
    }
    return 0;
}

#endif // __PTHREAD_H__
