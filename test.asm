# Test comment

push_i8 7; Another comment
push_i8 0x1F;

syscall hello_world;

jmp PAST_ADD;
add_i64;
PAST_ADD:

exit;
