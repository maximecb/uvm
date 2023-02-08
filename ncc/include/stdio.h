#ifndef __STDIO_H__
#define __STDIO_H__

int puts(char* str)
{
    asm (str) -> void { syscall print_str; };
    return 0;
}

#endif
