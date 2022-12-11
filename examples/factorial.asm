.data
PROMPT_STR: .stringz "Input an integer:"

.code

push_ptr32 PROMPT_STR;
syscall print_str;

# Read input number
syscall read_i64;

# Fall FACT with 1 argument
call 1, FACT;

# Print the result
syscall print_i64;

exit;

#### fact(n) ####
FACT:

dup;

# Check if n <= 1
dup;
push_i8 1;
le_i64;
jz RECURSE;
ret;

RECURSE:

dup;

# Compute fact(n-1)
push_i8 1;
sub_i64;
call 1, FACT;

# n * fact(n-1)
mul_i64;

ret;
