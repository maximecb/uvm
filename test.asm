# Data section
.data

# 800 * 600 * 3
PIXEL_BUFFER:
.zero 1_440_000

# Code section
.code

# TODO: need to specify window width and height
syscall window_create;

# TODO:
push_ptr32 PIXEL_BUFFER;
syscall window_copy_pixels;





exit;
