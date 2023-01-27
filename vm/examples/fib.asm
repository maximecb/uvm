#
# Recursive computation of Fibonacci numbers
#

.data;
PROMPT_STR: .stringz "Input an integer:\n";

# Code section
.code;

push PROMPT_STR;
syscall $PRINT_STR;

# Read input number
syscall $READ_I64;

# Fall FACT with 1 argument
call FIB, 1;

# Print the result
syscall $PRINT_I64;
syscall $PRINT_ENDL;

push 0;
exit;

#
# u64 fib(u64 n)
#
FIB:
get_arg 0;
push 2;
lt_i64;
jz _if_false_0;
get_arg 0;
ret;
_if_false_0:
get_arg 0;
push 1;
sub_u64;
call FIB, 1;
get_arg 0;
push 2;
sub_u64;
call FIB, 1;
add_u64;
ret;
