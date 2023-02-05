#include <assert.h>

u8 FRAME_BUFFER[1000];
char* CHAR_DOTS[128] = 0;

void main()
{
    CHAR_DOTS[48] = "foo";

    // Memset used to corrupt the memory used by CHAR_DOTS
    asm (FRAME_BUFFER, 0, 1000) -> void { syscall memset; };

    char* dots = CHAR_DOTS[48];
    assert(dots);
}
