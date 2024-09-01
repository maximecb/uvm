#include <assert.h>
#include <uvm/syscalls.h>

u64 thread_fn()
{
    thread_sleep(5);
    return 7;
}

int main()
{
    u64 tid = thread_spawn(thread_fn);
    u64 ret = thread_join(tid);
    assert(ret == 7);

    return 0;
}
