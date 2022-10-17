# Data section
.data





# Code section
.code

push_i8 7; Another comment
push_i8 77;
push_u64 0xFFFF;

syscall hello_world;

jmp PAST_ADD;
add_i64;
PAST_ADD:

exit;
