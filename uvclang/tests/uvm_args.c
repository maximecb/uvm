// Self-checking test that the entry helper forwards command-line arguments to
// main(argc, argv). Runs under run_uvm_tests.sh with no extra arguments, so the
// only guaranteed argument is argv[0] (the program path). We validate the shape
// of argc/argv rather than the exact contents, since argv[0] varies by run.
#include <assert.h>
#include <string.h>

int main(int argc, char** argv)
{
    // There is always at least the program name.
    assert(argc >= 1);

    // Every argv[i] for i < argc is a non-NULL, NUL-terminated string, and the
    // vector is NULL-terminated at argv[argc] (the C standard guarantee).
    for (int i = 0; i < argc; i++) {
        assert(argv[i] != 0);
        // strlen must terminate, proving the copied string has a NUL.
        volatile size_t len = strlen(argv[i]);
        (void)len;
    }
    assert(argv[argc] == 0);

    return 0;
}
