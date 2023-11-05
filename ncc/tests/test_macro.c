#ifndef TEST
This file fails to compile if the TEST macro
is not properly set when compiling tests.
i.e. ncc -DTEST
#endif

void main()
{
}
