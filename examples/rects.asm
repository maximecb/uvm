# Data section
.data;

# 800 * 600 * 3
PIXEL_BUFFER:
.zero 1_440_000;

WINDOW_TITLE:
.stringz "UVM Rectangles Example";

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

# void fill_rect(
#     u8* f_buffer,
#     u64 f_width,
#     u64 f_height,
#     u64 r_x,
#     u64 r_y,
#     u64 r_width,
#     u64 r_height,
#     u8 r,
#     u8 g,
#     u8 b,
# )
push PIXEL_BUFFER;
get_local 0;
get_local 1;
push 100;
push 100;
push 200;
push 100;
push 0;
push 0;
push 255;
call fill_rect, 10;

push PIXEL_BUFFER;
get_local 0;
get_local 1;
push 140;
push 140;
push 200;
push 100;
push 255;
push 0;
push 0;
call fill_rect, 10;

push PIXEL_BUFFER;
syscall window_copy_pixels;

wait;
exit;

#############################################################################

# void fill_rect(
#     u8* f_buffer,
#     u64 f_width,
#     u64 f_height,
#     u64 r_x,
#     u64 r_y,
#     u64 r_width,
#     u64 r_height,
#     u8 r,
#     u8 g,
#     u8 b,
# )
fill_rect:
push 0;
push 0;
push 0;
push 0;
dup;
set_local 0;
pop;
_for_loop_0:
get_local 0;
get_arg 6;
lt_i64;
jz _for_break_2;
push 0;
dup;
set_local 1;
pop;
_for_loop_3:
get_local 1;
get_arg 5;
lt_i64;
jz _for_break_5;
get_arg 0;
push 3;
get_arg 1;
mul_u64;
get_arg 4;
get_local 0;
add_u64;
mul_u64;
push 3;
get_arg 3;
get_local 1;
add_u64;
mul_u64;
add_u64;
add_u64;
dup;
set_local 2;
pop;
get_arg 7;
get_local 2;
push 0;
add_u64;
getn 1;
store_u8;
pop;
get_arg 8;
get_local 2;
push 1;
add_u64;
getn 1;
store_u8;
pop;
get_arg 9;
get_local 2;
push 2;
add_u64;
getn 1;
store_u8;
pop;
_for_cont_4:
get_local 1;
push 1;
add_u64;
dup;
set_local 1;
pop;
jmp _for_loop_3;
_for_break_5:
_for_cont_1:
get_local 0;
push 1;
add_u64;
dup;
set_local 0;
pop;
jmp _for_loop_0;
_for_break_2:
push 0;
ret;
