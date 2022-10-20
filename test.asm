# Data section
.data

.zero 32



# Code section
.code

push_i8 3;
push_i8 7; Another comment
push_i8 77;
push_u64 0xFFFF;
push_u32 0xFFFFFFFF;

syscall hello_world;

jmp PAST_ADD;
add_i64;
PAST_ADD:

exit;
