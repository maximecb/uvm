#ifndef __ASSERT_H__
#define __ASSERT_H__

// assert() macro
#ifndef NDEBUG
#define assert(test_val)\
if (!(test_val))\
{\
    asm () -> void { panic; };\
}
#else
#define assert(test_val) {}
#endif

#endif
