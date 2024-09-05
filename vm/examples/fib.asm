#
# Recursive computation of Fibonacci numbers
#

.data;
PROMPT_STR: .stringz "Input an integer:\n";

# Code section
.code;

push PROMPT_STR;
syscall print_str;

# Read input number
call READ_INT, 0;

# Call FIB with 1 argument
call FIB, 1;

# Print the result
syscall print_i64;
syscall print_endl;

push 0;
ret;

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

#
# Read a positive integer from stdlin
#
READ_INT:

push 0; # Current integer value

LOOP:
    # Read one character
    syscall getchar;

    # If < 0 done
    dup;
    push 48;
    lt_i64;
    jnz DONE;

    # If > 9 done
    dup;
    push 57;
    gt_i64;
    jnz DONE;

    # Convert to integer digit
    push 48;
    sub_u64;

    # int_val * 10;
    get_local 0;
    push 10;
    mul_u64;

    # int_val + 10;
    add_u64;
    set_local 0;

    jmp LOOP;

DONE:

get_local 0;
ret;
