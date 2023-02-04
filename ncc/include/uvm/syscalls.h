//
// This file was automatically generated based on api/syscalls.json
//

#ifndef __UVM_SYSCALLS__
#define __UVM_SYSCALLS__

// Copy a block of memory in the heap from a source address to a destination address.
inline void memcpy(u8* dst, const u8* src, u64 num_bytes)
{
    return asm (dst, src, num_bytes) -> void { syscall 3; };
}

// Fill a block of bytes in the heap with a given value.
inline void memset(u8* dst, u8 value, u64 num_bytes)
{
    return asm (dst, value, num_bytes) -> void { syscall 4; };
}

// Print an i64 value to standard output.
inline void print_i64(i64 val)
{
    return asm (val) -> void { syscall 5; };
}

// Print a string to standard output.
inline void print_str(const char* str)
{
    return asm (str) -> void { syscall 6; };
}

// Print a newline to standard output.
inline void print_endl()
{
    return asm () -> void { syscall 7; };
}

// Read an i64 value from standard input.
inline i64 read_i64()
{
    return asm () -> i64 { syscall 8; };
}

// Get the UNIX time stamp in milliseconds.
inline u64 time_current_ms()
{
    return asm () -> u64 { syscall 0; };
}

// Schedule a callback to be called once after a given delay.
inline void time_delay_cb(u64 delay_ms, void* callback)
{
    return asm (delay_ms, callback) -> void { syscall 2; };
}

// Create a new window with a frame buffer to draw into.
inline u32 window_create(u32 width, u32 height, const char* title, u64 flags)
{
    return asm (width, height, title, flags) -> u32 { syscall 1; };
}

// Show a window, initially not visible when created.
inline void window_show(u32 window_id)
{
    return asm (window_id) -> void { syscall 9; };
}

// Copy a frame of RGB24 pixels to be displayed into the window. The frame must have the same width and height as the window.
inline void window_draw_frame(u32 window_id, const u8* pixel_data)
{
    return asm (window_id, pixel_data) -> void { syscall 10; };
}

// Register a callback for mouse movement.
inline void window_on_mousemove(u32 window_id, void* callback)
{
    return asm (window_id, callback) -> void { syscall 11; };
}

// Register a callback for mouse button press events.
inline void window_on_mousedown(u32 window_id, void* callback)
{
    return asm (window_id, callback) -> void { syscall 12; };
}

// Register a callback for mouse button release events.
inline void window_on_mouseup(u32 window_id, void* callback)
{
    return asm (window_id, callback) -> void { syscall 13; };
}

#endif
