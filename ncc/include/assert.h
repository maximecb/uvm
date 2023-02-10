#ifndef __ASSERT_H__
#define __ASSERT_H__

// assert() macro
#ifndef NDEBUG
#define assert(test_val)\
if (!(test_val))\
{\
    asm ("assertion failed in ") -> void { syscall 6; };\
    asm (__FILE__) -> void { syscall 6; };\
    asm ("@") -> void { syscall 6; };\
    asm (__LINE__) -> void { syscall 5; };\
    asm () -> void { syscall 7; };\
    asm () -> void { panic; };\
}
#else
#define assert(test_val) {}
#endif

// todo() macro for unimplemented features
#define todo()\
{\
    asm ("not yet implemented") -> void { syscall 6; };\
}

#endif
