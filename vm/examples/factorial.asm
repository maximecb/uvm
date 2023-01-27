/*
* A little program to recursively compute factorials
*/

.data;
PROMPT_STR: .stringz "Input an integer:\n";

# Code section
.code;

push PROMPT_STR;
syscall $PRINT_STR;

# Read input number
syscall $READ_I64;

# Fall FACT with 1 argument
call FACT, 1;

# Print the result
syscall $PRINT_I64;
syscall $PRINT_ENDL;

push 0;
exit;

#### fact(n) ####
FACT:

# Check if n <= 1
get_arg 0;
push 1;
le_i64;
jz RECURSE;
get_arg 0;
ret;

RECURSE:

# Compute fact(n-1)
get_arg 0;
push 1;
sub_u64;
call FACT, 1;

# n * fact(n-1)
get_arg 0;
mul_u64;

ret;
