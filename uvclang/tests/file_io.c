// Differential test for the <stdio.h> FILE* streams (fopen/fclose/fread/
// fwrite/fseek/ftell/feof) referenced by the header's file-streams section:
// uvclang's UVM-side file I/O (thin wrappers over the file_* syscalls) vs the
// platform's native libc. Both must produce byte-identical stdout and the
// same exit code.
//
// The test writes/reads a fixed relative file so it behaves the same for the
// native reference binary and for uvm, which both run with the uvclang/
// directory as their current working directory (see run_tests.sh). The file
// is not cleaned up afterwards (uvclang has no remove()/unlink()); it is
// listed in .gitignore.
#include <stdio.h>
#include <string.h>

static const char *PATH = "file_io_test.tmp";

int main()
{
    // --- write ---
    FILE *f = fopen(PATH, "wb");
    if (!f) { puts("fopen(wb) failed"); return 1; }
    const char *msg = "Hello, UVM file I/O!";  // 20 bytes
    size_t wrote = fwrite(msg, 1, strlen(msg), f);
    printf("wrote %lu bytes\n", (unsigned long)wrote);
    fclose(f);

    // --- read back, past EOF ---
    f = fopen(PATH, "rb");
    if (!f) { puts("fopen(rb) failed"); return 1; }
    char buf[64] = {0};
    size_t got = fread(buf, 1, sizeof(buf) - 1, f);
    printf("read %lu bytes: %s\n", (unsigned long)got, buf);
    printf("feof after short read=%d\n", feof(f) != 0);

    // --- fseek/ftell: SEEK_SET, SEEK_END, SEEK_CUR ---
    fseek(f, 7, SEEK_SET);
    printf("feof after seek=%d\n", feof(f) != 0);
    char mid[6] = {0};
    fread(mid, 1, 5, f);
    printf("seek(SET,7) read: %s, tell=%ld\n", mid, ftell(f));

    fseek(f, -5, SEEK_END);
    printf("seek(END,-5) tell=%ld\n", ftell(f));
    fseek(f, 2, SEEK_CUR);
    printf("seek(CUR,+2) tell=%ld\n", ftell(f));
    fclose(f);

    // --- append ---
    f = fopen(PATH, "ab");
    if (!f) { puts("fopen(ab) failed"); return 1; }
    fwrite(" more", 1, 5, f);
    fclose(f);

    f = fopen(PATH, "rb");
    char buf2[64] = {0};
    size_t total = fread(buf2, 1, sizeof(buf2) - 1, f);
    printf("after append %lu bytes: %s\n", (unsigned long)total, buf2);
    fclose(f);

    // --- opening a nonexistent file fails cleanly ---
    FILE *missing = fopen("this_file_should_not_exist.tmp", "rb");
    printf("fopen(missing) -> %s\n", missing ? "opened" : "NULL");

    return 0;
}
