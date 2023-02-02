#define assert(test_val)\
if (!test_val)\
{\
    print_str("assertion failed");\
    print_endl();\
    asm () -> void { panic; } \
}
