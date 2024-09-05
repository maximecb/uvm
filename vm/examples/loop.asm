#
# This is a microbenchmark with a loop and some timing functions to
# try and calculate how many millions of instructions per second (MIPS)
# we can run.
#

.data;
MIPS_STR: .stringz " MIPS (million insns/second)";

.code;

# Counter, local 0
push 0;

# End count, local 1
push 10_000_000;

# Start time in ms, local 2
syscall time_current_ms;

# 9 instructions per iteration
LOOP:

# Test
get_local 0;
get_local 1;
lt_i64; # l0 < COUNT
jz DONE;

# Increment
get_local 0;
push 1;
add_u64;
set_local 0;

# Loop
jmp LOOP;

DONE:

# End time in ms, local 3
syscall time_current_ms;

# Compute time taken
get_local 3;
get_local 2;
sub_u64;

# Compute instructions/second
get_local 1; # Total iteration count
push 9; # 9 instructions per cycle
mul_u64; # Iterations * insns = total insn executed
swap;
div_i64; # Insns / time taken in milliseconds
push 1000;
mul_u64; # Insns / second
push 1_000_000;
div_i64; # MIPS (M insns / second)
syscall print_i64;
push MIPS_STR;
syscall print_str;
syscall print_endl;

ret;
