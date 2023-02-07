#ifndef __STDLIB_H__
#define __STDLIB_H__

void exit(int status)
{
    asm (status) -> void { exit; };
}

#endif
