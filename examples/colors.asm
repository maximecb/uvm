# Data section
.data;

# 800 * 600 * 3
PIXEL_BUFFER:
.zero 1_440_000;

WINDOW_TITLE:
.stringz "UVM Color Band Example";

#############################################################################

# Code section
.code;

# Local 0, 1, width and height
push 800;
push 600;

# Create the window
get_local 0;
get_local 1;
push WINDOW_TITLE;
syscall window_create;
syscall window_show;

# 3^3 = 27, 22 pixels per band
# We want to map 3 groups of 3 to [0, 127, 255]
# 127 * i where i is 0, 1, 2

# Current color index (local 2)
push 0;

# For each color
COLOR_LOOP:

# ymin
get_local 2;
push 22;
mul_u64;

# ymax
dup;
push 22;
add_u64;

# R color = (i % 3) % 127
get_local 2;
push 3;
mod_i64;
push 127;
mul_u64;

# R color = ((i/3) % 3) % 127
get_local 2;
push 3;
div_i64;
push 3;
mod_i64;
push 127;
mul_u64;

# G color = ((i/9) % 3) % 127
get_local 2;
push 9;
div_i64;
push 3;
mod_i64;
push 127;
mul_u64;

# DRAW_COLOR_BAND(ymin, ymax, r, g, b)
call DRAW_COLOR_BAND, 5;
pop;

# Increment color index
get_local 2;
push 1;
add_u64;
set_local 2;

# Loop until done with colors
get_local 2;
push 27;
lt_i64;
jnz COLOR_LOOP;

push PIXEL_BUFFER;
syscall window_copy_pixels;

# Wait for an event
wait;

exit;

#############################################################################

# DRAW_COLOR_BAND(ymin, ymax, r, g, b)
DRAW_COLOR_BAND:

# Compute the start address (local 0)
get_arg 0;
push 2400;
mul_u64; # ymin * 800 * 3

# Compute the end address (local 1)
get_arg 1;
push 2400;
mul_u64; # ymax * 800 * 3

# For each pixel
PIXEL_LOOP:

# Write R value
get_local 0;
get_arg 2;
store_u8;

# Increment value address
get_local 0;
push 1;
add_u64;
set_local 0;

# Write G value
get_local 0;
get_arg 3;
store_u8;

# Increment value address
get_local 0;
push 1;
add_u64;
set_local 0;

# Write B value
get_local 0;
get_arg 4;
store_u8;

# Increment value address
get_local 0;
push 1;
add_u64;
set_local 0;

# Loop until done writing pixels
get_local 0;
get_local 1;
lt_i64;
jnz PIXEL_LOOP;

push 0;
ret;
