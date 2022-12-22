# Data section
.data;

# 800 * 600 * 3
PIXEL_BUFFER:
.zero 1_440_000;

WINDOW_TITLE:
.stringz "UVM Circle Animation Example";

# Global x coordinate variable
X_COORD:
.u64 0;

###########################################################

# Code section
.code;

# Create a window
push 800;
push 600;
push WINDOW_TITLE;
syscall window_create;

# Show the window
syscall window_show;



push 100;
push ANIM_CALLBACK;
syscall time_delay_cb;





push PIXEL_BUFFER;
syscall window_copy_pixels;




# Wait for an event
wait;

###########################################################

# Animation callback
ANIM_CALLBACK:

# x: local 0
push X_COORD;
load_u64;

get_local 0;
syscall print_i64;
syscall print_endl;
syscall print_endl;

# x = x + dx
get_local 0;
push 50;
add_u64;
set_local 0;

# x % 600
get_local 0;
push 600;
mod_i64;
set_local 0;

# update global x variable
push X_COORD;
get_local 0;
store_u64;

# x += 100
get_local 0;
push 100;
add_u64;
set_local 0;

# Clear the screen
push PIXEL_BUFFER;
push 0;
push 1_440_000;
syscall memset;

# Draw the circle
get_local 0;
push 300;
push 20;
call DRAW_CIRCLE, 3;
pop;

push PIXEL_BUFFER;
syscall window_copy_pixels;

# Schedule the animation callback again
push 100;
push ANIM_CALLBACK;
syscall time_delay_cb;

pop;
wait;

###########################################################

# Draw a circle
# DRAW_CIRCLE(x, y, r)
DRAW_CIRCLE:

# Local 0
# xmin = x - r
get_arg 0;
get_arg 2;
sub_u64;

# Local 1
# xmax = x + r
get_arg 0;
get_arg 2;
add_u64;

# Local 2
# ymin = y - r
get_arg 1;
get_arg 2;
sub_u64;

# Local 3
# ymax = y + r
get_arg 1;
get_arg 2;
add_u64;

# Local 4: x
# x = 0
push 0;

# Local 5: y
# y = ymin
get_local 2;

# For each row
LOOP_Y:

    # For each column
    # x = xmin
    get_local 0;
    set_local 4;
    LOOP_X:

    # (x - xin)^2
    get_local 4;
    get_arg 0;
    sub_u64;
    dup;
    mul_u64;

    # (y - yin)^2
    get_local 5;
    get_arg 1;
    sub_u64;
    dup;
    mul_u64;

    # dx^2 + dy^2
    add_u64;

    # r^2
    get_arg 2;
    dup;
    mul_u64;

    # dx^2 + dy^2 < r^2
    lt_i64;
    jz OUTSIDE_CIRCLE;

    get_local 4;
    get_local 5;
    call SET_PIXEL, 2;
    pop;

    OUTSIDE_CIRCLE:

    # x = x + 2
    get_local 4;
    push 1;
    add_u64;
    set_local 4;

    # while (x < xmax)
    get_local 4;
    get_local 1;
    lt_i64;
    jnz LOOP_X;

# y = y + 1
get_local 5;
push 1;
add_u64;
set_local 5;

# while (y < ymax)
get_local 5;
get_local 3;
lt_i64;
jnz LOOP_Y;

push 0;
ret;

###########################################################

# Set a pixel red
# SET_PIXEL(x, y)
SET_PIXEL:

# Compute the pixel's address
# 800 * 3 * y + 3 * x
push 2400;
get_arg 1;
mul_u64;
get_arg 0;
push 3;
mul_u64;
add_u64;

push 255;
store_u8;

push 0;
ret;
