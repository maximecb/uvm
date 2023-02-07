#
# This is a microbenchmark to test how many frames of pixels we could
# copy per second using memcpy to copy each row of pixels eventually.
# It's not truly fair because of caching, but it shows that we're
# not limited by interpreter overhead.
#

.data;
FPS_STR: .stringz " FPS (frames/second)";

# Array of 1000 RGBA pixels (one row)
SRC_BUFFER:
.zero 4_000;
DST_BUFFER:
.zero 4_000;

.code;

# Counter, local 0
push 0;

# End count, local 1
push 10_000_000;

# Start time in ms, local 2
syscall time_current_ms;

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

# Copy one row of pixels using memcpy
push DST_BUFFER;
push SRC_BUFFER;
push 4000;
syscall memcpy;

# Loop
jmp LOOP;

DONE:

# End time in ms, local 3
syscall time_current_ms;

# Compute time taken
get_local 3;
get_local 2;
sub_u64;

get_local 1; # Total row count
push 1000; # 1000 rows per frame
div_i64;

swap;
div_i64; # Frames / time taken in ms

push 1000;
mul_u64; # Frames / second

syscall print_i64;
push FPS_STR;
syscall print_str;
syscall print_endl;

exit;
