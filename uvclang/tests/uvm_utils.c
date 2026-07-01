// Self-checking test for <uvm/utils.h>: the benchmark() macro times a loop with
// the millisecond-clock syscall and prints "<elapsed> ms". The elapsed time is
// nondeterministic, so only the exit code is checked (the run must not crash).
#include <uvm/utils.h>

int main()
{
    volatile int n = 1000;
    volatile int x = 0;

    benchmark({
        for (int i = 0; i < n; i++)
            x = x + i;
    });

    return 0;
}
