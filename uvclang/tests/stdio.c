// Differential test for <stdio.h>: uvclang's UVM-side stdio (character/string
// output over UVM syscalls) vs the platform's native libc. The UVM build
// resolves <stdio.h> to uvclang/include/stdio.h; the native reference build uses
// the system header. Both must produce byte-identical stdout and the same exit
// code.
//
// Only the output primitives are exercised: puts() and putchar(). printf() is
// not yet ported (needs callee-side varargs) and getchar() is not exercised (the
// harness feeds no stdin). A volatile seed keeps -O2 from const-folding the
// loop into a single string.
#include <stdio.h>

int main()
{
    volatile int seed = 5;

    puts("hello from stdio");        // "hello from stdio\n"

    // Print a run of characters with putchar.
    for (int i = 0; i < seed; i++)
        putchar('a' + i);            // "abcde"
    putchar('\n');

    // putchar returns the character written; fold it into the exit code.
    int r = putchar('X');            // prints 'X', returns 'X' == 88
    putchar('\n');

    return r & 0x7F;                 // 88
}
