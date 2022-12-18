.data
MIPS_STR: .stringz " MIPS (million insns/second)"

.code

# Counter, local 0
push_i8 0;

# End count, local 1
push_u64 10_000_000;

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
push_i8 1;
add_i64;
set_local 0;

# Loop
jmp LOOP;

DONE:

# End time in ms, local 3
syscall time_current_ms;

# Compute time taken
get_local 3;
get_local 2;
sub_i64;

# Compute instructions/second
get_local 1; # Total iteration count
push_i8 9; # 9 instructions per cycle
mul_i64; # Iterations * insns = total insn executed
swap;
div_i64; # Insns / time taken in milliseconds
push_u64 1000;
mul_i64; # Insns / second
push_u64 1_000_000;
div_i64; # MIPS (M insns / second)
syscall print_i64;
push_ptr32 MIPS_STR;
syscall print_str;
syscall print_endl;

exit;
