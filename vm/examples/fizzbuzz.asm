.data;
FIZZ: .stringz "Fizz\n";
BUZZ: .stringz "Buzz\n";
FIZZBUZZ: .stringz "FizzBuzz\n";

.code;

# Counter, local 0
push 1;

LOOP:

# if (i%15==0) print("fizzbuzz\n");
get_local 0;
push 15;
mod_i64;
jnz NOT_FIZZBUZZ;
push FIZZBUZZ;
syscall print_str;
jmp INCR;
NOT_FIZZBUZZ:

# if (i%3==0) print("fizz\n");
get_local 0;
push 3;
mod_i64;
jnz NOT_FIZZ;
push FIZZ;
syscall print_str;
jmp INCR;
NOT_FIZZ:

# if (i%5==0) print("buzz\n");
get_local 0;
push 5;
mod_i64;
jnz NOT_BUZZ;
push BUZZ;
syscall print_str;
jmp INCR;
NOT_BUZZ:

# Print the number
get_local 0;
syscall print_i64;
syscall print_endl;

# Increment
INCR:
get_local 0;
push 1;
add_u64;
set_local 0;

# Test
get_local 0;
push 101;
lt_i64; # l0 < COUNT
jnz LOOP;

exit;
