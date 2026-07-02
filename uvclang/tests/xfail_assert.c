// Self-checking test for the <assert.h> FAILURE path -- the one path the
// differential and uvm_* harnesses never exercise (every other test asserts only
// true predicates). A failed assert must print an "assertion failed" diagnostic
// and terminate with a non-zero exit code.
//
// This can't be a differential test: native libc routes a failure through abort()
// (SIGABRT, message to stderr, exit 134), while uvclang's <assert.h> prints to
// stdout and calls __uvm_exit(-1) (exit 255). They agree only that the program
// terminates abnormally after complaining. So it runs under the harness's
// `xfail_*` convention: compile+run only the UVM build and require a NON-zero exit
// with the diagnostic on stdout.
//
// A volatile zero keeps clang from proving the predicate false at compile time --
// otherwise -O2 could drop the message formatting and turn main into a bare trap.
#include <assert.h>

int main()
{
    volatile int zero = 0;
    assert(zero == 1 && "assert failure path should fire");
    return 0;   // unreachable: the assert above terminates the program
}
