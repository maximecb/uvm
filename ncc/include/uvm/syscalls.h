//
// This file was automatically generated based on api/syscalls.json
//

#ifndef __UVM_SYSCALLS__
#define __UVM_SYSCALLS__

// void memcpy(u8* dst, const u8* src, u64 num_bytes)
// Copy a block of memory in the heap from a source address to a destination address.
#define memcpy(__dst, __src, __num_bytes) asm (__dst, __src, __num_bytes) -> void { syscall memcpy; }

// void memset(u8* dst, u8 value, u64 num_bytes)
// Fill a block of bytes in the heap with a given value.
#define memset(__dst, __value, __num_bytes) asm (__dst, __value, __num_bytes) -> void { syscall memset; }

// u64 vm_heap_size()
// Report the current heap size in bytes.
#define vm_heap_size() asm () -> u64 { syscall vm_heap_size; }

// void print_i64(i64 val)
// Print an i64 value to standard output.
#define print_i64(__val) asm (__val) -> void { syscall print_i64; }

// void print_str(const char* str)
// Print a string to standard output.
#define print_str(__str) asm (__str) -> void { syscall print_str; }

// void print_endl()
// Print a newline to standard output.
#define print_endl() asm () -> void { syscall print_endl; }

// i64 read_i64()
// Read an i64 value from standard input.
#define read_i64() asm () -> i64 { syscall read_i64; }

// u64 time_current_ms()
// Get the UNIX time stamp in milliseconds.
#define time_current_ms() asm () -> u64 { syscall time_current_ms; }

// void time_delay_cb(u64 delay_ms, void* callback)
// Schedule a callback to be called once after a given delay.
#define time_delay_cb(__delay_ms, __callback) asm (__delay_ms, __callback) -> void { syscall time_delay_cb; }

// u32 window_create(u32 width, u32 height, const char* title, u64 flags)
// Create a new window with a frame buffer to draw into.
#define window_create(__width, __height, __title, __flags) asm (__width, __height, __title, __flags) -> u32 { syscall window_create; }

// void window_draw_frame(u32 window_id, const u8* pixel_data)
// Copy a frame of pixels to be displayed into the window. The frame must have the same width and height as the window. The pixel format is 32 bits per pixel in BGRA byte order, with 8 bits for each component and the B byte at the lowest address.
#define window_draw_frame(__window_id, __pixel_data) asm (__window_id, __pixel_data) -> void { syscall window_draw_frame; }

// void window_on_mousemove(u32 window_id, void* callback)
// Register a callback for mouse movement.
#define window_on_mousemove(__window_id, __callback) asm (__window_id, __callback) -> void { syscall window_on_mousemove; }

// void window_on_mousedown(u32 window_id, void* callback)
// Register a callback for mouse button press events.
#define window_on_mousedown(__window_id, __callback) asm (__window_id, __callback) -> void { syscall window_on_mousedown; }

// void window_on_mouseup(u32 window_id, void* callback)
// Register a callback for mouse button release events.
#define window_on_mouseup(__window_id, __callback) asm (__window_id, __callback) -> void { syscall window_on_mouseup; }

// void window_on_keydown(u32 window_id, void* callback)
// Register a callback for key press event.
#define window_on_keydown(__window_id, __callback) asm (__window_id, __callback) -> void { syscall window_on_keydown; }

// void window_on_keyup(u32 window_id, void* callback)
// Register a callback for key release event.
#define window_on_keyup(__window_id, __callback) asm (__window_id, __callback) -> void { syscall window_on_keyup; }

#define KEY_BACKSPACE 8
#define KEY_TAB 9
#define KEY_RETURN 10
#define KEY_ESCAPE 27
#define KEY_SPACE 32
#define KEY_NUM0 48
#define KEY_A 65
#define KEY_B 66
#define KEY_C 67
#define KEY_D 68
#define KEY_S 83
#define KEY_W 87
#define KEY_LEFT 16001
#define KEY_RIGHT 16002
#define KEY_UP 16003
#define KEY_DOWN 16004
#define KEY_SHIFT 16005

#endif
