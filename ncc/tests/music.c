#include <uvm/music.h>

int main()
{
    float a4_f = pc_to_freq(A4_NOTE_NO, 0);
    assert(fabsf(a4_f - 440) < 0.1f);

    return 0;
}
