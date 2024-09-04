#include <assert.h>
#include <uvm/syscalls.h>

u64 thread_fn(u64 arg)
{
    thread_sleep(5);
    return arg  + 1;
}

int main()
{
    u64 tid = thread_spawn(thread_fn, 7);
    u64 ret = thread_join(tid);
    assert(ret == 8);

    return 0;
}
