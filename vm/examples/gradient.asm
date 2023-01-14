# Data section
.data;

# 800 * 600 * 3
PIXEL_BUFFER:
.zero 1_440_000;

WINDOW_TITLE:
.stringz "UVM Gradient Example";

MS_STR:
.stringz " milliseconds to render\n";

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

# Local 2: Y=0
push 0;

# Local 3: X=0
push 0;

# Start time in ms, local 4
syscall time_current_ms;

# For each row
LOOP_Y:

    # For each column
    # X = 0
    push_i8 0;
    set_local 3;
    LOOP_X:

    # Pixel address
    get_local 2; # Y
    push_u64 800;
    mul_u64;
    get_local 3;
    add_u64; # Y * 800 + X
    push_u64 3;
    mul_u64; # (Y * 800 + X) * 3
    dup;

    # Y * 256 / 600
    get_local 2;
    push_u64 256;
    mul_u64;
    push_u64 600;
    div_i64;
    store_u8;

    # Blue coordinate address
    push_i8 2;
    add_u64;

    # X * 256 / 800
    get_local 3;
    push_u64 256;
    mul_u64;
    push_u64 800;
    div_i64;
    store_u8;

    # X = X + 1
    get_local 3;
    push_i8 1;
    add_u64;
    set_local 3;

    # Loop until done writing pixels
    get_local 3;
    get_local 0;
    lt_i64;
    jnz LOOP_X;

# Y = Y + 1
get_local 2;
push_i8 1;
add_u64;
set_local 2;

# Loop for each row
get_local 2;
get_local 1;
lt_i64;
jnz LOOP_Y;

# End time in ms
syscall time_current_ms;

# Compute render time in ms
get_local 4;
sub_u64;

syscall print_i64;
push MS_STR;
syscall print_str;

push PIXEL_BUFFER;
syscall window_copy_pixels;

# Return to the event loop
push 0;
ret;
