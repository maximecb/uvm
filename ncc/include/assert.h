#ifndef __ASSERT_H__
#define __ASSERT_H__

// assert() macro
#ifndef NDEBUG
#define assert(test_val) if (!(test_val)) { unsigned int line_no = __LINE__; \
    asm ("assertion failed in ") -> void { syscall print_str; };\
    asm (__FILE__) -> void { syscall print_str; };\
    asm ("@") -> void { syscall print_str; };\
    asm (line_no) -> void { syscall print_i64; };\
    asm () -> void { syscall print_endl; };\
    asm () -> void { panic; };\
}
#else
#define assert(test_val) {}
#endif

// TODO: add line numbers
// todo() macro for unimplemented features
#define todo()\
{\
    asm ("not yet implemented") -> void { syscall 6; };\
}

// TODO: add line numbers
// panic(msg) macro
#define panic(msg)\
{\
    asm (msg) -> void { syscall 6; panic; };\
}

#endif
