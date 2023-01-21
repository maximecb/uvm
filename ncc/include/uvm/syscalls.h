//
// This file was automatically generated based on api/syscalls.json
//

// Copy a block of memory in the heap from a source address to a destination address
inline void memcpy(u8* dst, const u8* src, u64 num_bytes)
{
    return syscall (dst, src, num_bytes) -> void { syscall 3; };
}

// Fill a block of bytes with a given value
inline void memset(u8* dst, u8 value, u64 num_bytes)
{
    return syscall (dst, value, num_bytes) -> void { syscall 4; };
}

// Print an i64 value to standard output
inline void print_i64(i64 val)
{
    return syscall (val) -> void { syscall 5; };
}

// Print a string to standard output
inline void print_str(const char* str)
{
    return syscall (str) -> void { syscall 6; };
}

// Print a newline to standard output
inline void print_endl()
{
    return syscall () -> void { syscall 7; };
}

// Read an i64 value from standard input
inline i64 read_i64()
{
    return syscall () -> i64 { syscall 8; };
}

// Get the UNIX time stamp in milliseconds
inline u64 time_current_ms()
{
    return syscall () -> u64 { syscall 0; };
}

// Schedule a callback to be called once after a given delay
inline void time_delay_cb(u64 delay_ms, u64 callback_pc)
{
    return syscall (delay_ms, callback_pc) -> void { syscall 2; };
}

// Create a new window with a frame buffer to draw into
inline u32 window_create(u32 width, u32 height, const char* title, u64 flags)
{
    return syscall (width, height, title, flags) -> u32 { syscall 1; };
}

