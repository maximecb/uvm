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
// recursive/error-checking mutex and no condition variables; the only thread
// attribute honored is the private stack size (pthread_attr_setstacksize).
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

// Returned by pthread_create when a thread stack could not be allocated.
#ifndef EAGAIN
#define EAGAIN 11
#endif

// Returned by pthread_attr_setstacksize for a size below PTHREAD_STACK_MIN.
#ifndef EINVAL
#define EINVAL 22
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
// Each guest thread runs on its own software stack for `alloca`/address-taken
// locals: a shared thread pointer would let concurrent threads' frames overlap
// and corrupt each other. The main thread (and any thread the runtime spawns,
// e.g. audio callbacks) uses a shared fallback stack; threads created here get
// a *private* stack, installed by the compiler's frame-free trampoline
// (__uvclang_thread_start) before the thread's first frame runs.
//
// The private stack, plus the control block the trampoline reads, live in one
// malloc'd region: pthread_create allocates it and pthread_join frees it.

#include <stdlib.h>          // malloc / free

// Default per-thread private stack size, used when pthread_create is passed a
// NULL attr (or an attr left at its initialized default). 256 KiB is plenty for
// typical call depth; deeply recursive threads can request more via a
// pthread_attr_t (see pthread_attr_setstacksize).
#define PTHREAD_STACK_SIZE (256 * 1024)

// Smallest private stack a thread may be created with. Requests below this are
// rejected by pthread_attr_setstacksize, mirroring POSIX PTHREAD_STACK_MIN: a
// floor beneath which even a trivial start routine cannot be expected to run.
#ifndef PTHREAD_STACK_MIN
#define PTHREAD_STACK_MIN (16 * 1024)
#endif

// -- thread attributes ----------------------------------------------------
//
// A minimal attribute object carrying just the private stack size, the only
// pthread_create knob that means anything on this target (there is no scheduling
// policy, CPU affinity, or guard-page control to express here). A freshly
// initialized attr requests the default PTHREAD_STACK_SIZE; passing NULL as
// pthread_create's attr has the same effect.
typedef struct
{
    size_t stacksize;   // bytes of private stack for the created thread
} pthread_attr_t;

// Initialize an attribute object to the defaults. attr must be non-NULL.
// Always returns 0.
UVCLANG_WEAK int pthread_attr_init(pthread_attr_t* attr)
{
    attr->stacksize = PTHREAD_STACK_SIZE;
    return 0;
}

// Destroy an attribute object. Nothing to release; provided for API symmetry.
// Always returns 0.
UVCLANG_WEAK int pthread_attr_destroy(pthread_attr_t* attr)
{
    (void)attr;
    return 0;
}

// Set the private stack size a subsequent pthread_create(&attr, ...) will use.
// Returns EINVAL, leaving the attr unchanged, if stacksize is below
// PTHREAD_STACK_MIN (as POSIX specifies); otherwise stores it and returns 0.
UVCLANG_WEAK int pthread_attr_setstacksize(pthread_attr_t* attr, size_t stacksize)
{
    if (stacksize < PTHREAD_STACK_MIN) {
        return EINVAL;
    }
    attr->stacksize = stacksize;
    return 0;
}

// Read back the stack size stored in an attribute object. Always returns 0.
UVCLANG_WEAK int pthread_attr_getstacksize(const pthread_attr_t* attr, size_t* stacksize)
{
    *stacksize = attr->stacksize;
    return 0;
}

// Control block, placed at the base of each thread's region. The field order
// and offsets are fixed by the compiler's __uvclang_thread_start trampoline:
// stack@0, start@8, arg@16, tid@24. `stack` points past this header, where the
// thread's frames begin.
typedef struct __uvclang_thread
{
    void* stack;              // installed as the thread's stack pointer
    void* (*start)(void*);    // user start routine
    void* arg;                // user argument
    uint64_t tid;             // UVM thread id, for join
} *pthread_t;

// The compiler-synthesized, frame-free thread entry trampoline. Referenced by
// address only (never called from C); its body is emitted by uvclang.
extern void* __uvclang_thread_start(void*);

// Bytes reserved at the base of a region for the control block before the
// usable stack begins (16-aligned to keep frame alignment sane).
#define __UVCLANG_STACK_HEADER 64

// Start a new thread running start(arg) on a private stack. attr is ignored. On
// success writes an opaque handle through `thread` (if non-NULL) and returns 0,
// or a nonzero error if the stack could not be allocated.
UVCLANG_WEAK int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start)(void*), void* arg)
{
    // Resolve the private stack size: from the attribute object when one is
    // supplied (and left non-zero), otherwise the default. Enforce the same
    // floor as pthread_attr_setstacksize so a hand-zeroed attr can't undershoot.
    size_t stacksize = (attr && attr->stacksize) ? attr->stacksize : PTHREAD_STACK_SIZE;
    if (stacksize < PTHREAD_STACK_MIN) {
        stacksize = PTHREAD_STACK_MIN;
    }

    void* region = malloc(__UVCLANG_STACK_HEADER + stacksize);
    if (!region) {
        return EAGAIN;
    }

    pthread_t t = (pthread_t)region;
    t->stack = (char*)region + __UVCLANG_STACK_HEADER;
    t->start = start;
    t->arg = arg;
    t->tid = thread_spawn((void*)__uvclang_thread_start, t);

    if (thread) {
        *thread = t;
    }
    return 0;
}

// Wait for `thread` to finish, then free its stack region. If retval is
// non-NULL, the thread's return value is stored there. Returns 0.
UVCLANG_WEAK int pthread_join(pthread_t thread, void** retval)
{
    uint64_t r = thread_join(thread->tid);
    if (retval) {
        *retval = (void*)(uintptr_t)r;
    }
    // The thread has fully exited (thread_join waited for it), so its stack is
    // idle and safe to release.
    free(thread);
    return 0;
}

#endif // __PTHREAD_H__
