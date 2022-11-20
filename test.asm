# Data section
.data

# 800 * 600 * 3
PIXEL_BUFFER:
.zero 1_440_000

# Code section
.code

push_i8 77;
syscall print_i64;

# TODO: need to specify window width and height
syscall window_create;





# Draw a little white line

push_i8 0; # Start address

LOOP:

# Move one pixel to the right
push_i8 3;
add_i64;

# Write one component value
dup;
push_u64 255;
store_u8;

# Loop until done writing pixels
dup;
push_u64 765;
lt_i64;
jnz LOOP;











push_ptr32 PIXEL_BUFFER;
syscall window_copy_pixels;

syscall window_show;

exit;
