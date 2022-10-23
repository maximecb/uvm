# Data section
.data

# 800 * 600 * 3
PIXEL_BUFFER:
.zero 1_440_000

# Code section
.code




syscall window_create;




# TODO:
#syscall window_copy_pixels





exit;
