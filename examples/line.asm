# Data section
.data

# 800 * 600 * 3
PIXEL_BUFFER:
.zero 1_440_000

# Code section
.code

push_u32 600;
push_u32 800;
syscall window_create;


push_i8 20;
push_i8 20;
push_i8 10;
call 3, DRAW_HLINE;

push_i8 20;
push_i8 20;
push_i8 10;
call 3, DRAW_VLINE;






push_p32 PIXEL_BUFFER;
syscall window_copy_pixels;

syscall window_show;

# Wait for an event
wait;

exit;




###########################################################

# DRAW_HLINE(x, y, w)
DRAW_HLINE:

# Compute the start address
# 800 * 3 * y + 3 * x
push_u32 2400;
get_arg 1;
mul_i64;
get_arg 0;
push_i8 3;
mul_i64;
add_i64;

# Compute the end address
get_local 0;
get_arg 2;
push_i8 3;
mul_i64;
add_i64;

LOOP_H:

# Write one component value
get_local 0;
push_u64 255;
store_u8;

# Move one pixel to the right
get_local 0;
push_i8 3;
add_i64;
set_local 0;

# Loop until done writing pixels
get_local 0;
get_local 1;
lt_i64;
jnz LOOP_H;

ret;

###########################################################

# DRAW_VLINE(x, y, w)
DRAW_VLINE:

# Compute the start address
# 800 * 3 * y + 3 * x
push_u32 2400;
get_arg 1;
mul_i64;
get_arg 0;
push_i8 3;
mul_i64;
add_i64;

# Compute the end address
get_local 0;
get_arg 2;
push_u64 2400;
mul_i64;
add_i64;

LOOP_V:

# Write one component value
get_local 0;
push_u64 255;
store_u8;

# Move one pixel down
get_local 0;
push_u64 2400;
add_i64;
set_local 0;

# Loop until done writing pixels
get_local 0;
get_local 1;
lt_i64;
jnz LOOP_V;

ret;
