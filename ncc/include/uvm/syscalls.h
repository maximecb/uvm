//
// This file was automatically generated based on api/syscalls.json
//

#ifndef __UVM_SYSCALLS__
#define __UVM_SYSCALLS__

// void memset(u8* dst, u8 value, u64 num_bytes)
// Fill a block of bytes in the heap with a given value.
#define memset(__dst, __value, __num_bytes) asm (__dst, __value, __num_bytes) -> void { syscall memset; }

// void memset32(u32* dst, u32 word, u64 num_words)
// Fill a region of memory with 32-bit values. This is useful for some graphics operations.
#define memset32(__dst, __word, __num_words) asm (__dst, __word, __num_words) -> void { syscall memset32; }

// void memcpy(u8* dst, const u8* src, u64 num_bytes)
// Copy a block of memory in the heap from a source address to a destination address.
#define memcpy(__dst, __src, __num_bytes) asm (__dst, __src, __num_bytes) -> void { syscall memcpy; }

// i32 memcmp(const u8* p_a, const u8* p_b, u64 num_bytes)
// Compare two sequences of bytes. Returns 0 if equal, -1 if the first mismatching byte has a lower value in `p_a`, 1 if greater.
#define memcmp(__p_a, __p_b, __num_bytes) asm (__p_a, __p_b, __num_bytes) -> i32 { syscall memcmp; }

// u64 vm_heap_size()
// Report the current heap size in bytes.
#define vm_heap_size() asm () -> u64 { syscall vm_heap_size; }

// u64 vm_grow_heap(u64 num_bytes)
// Grow the heap to a new size given in bytes. This is similar to the `brk()` system call on POSIX systems. Note that the heap may be resized to a size larger than requested. The heap size is guaranteed to be a multiple of 8 bytes. If the requested size is smaller than the current heap size, this is a no-op. Returns the new heap size in bytes.
#define vm_grow_heap(__num_bytes) asm (__num_bytes) -> u64 { syscall vm_grow_heap; }

// u64 thread_spawn(void* fptr)
// Spawn a new thread running the given function.
#define thread_spawn(__fptr) asm (__fptr) -> u64 { syscall thread_spawn; }

// u64 thread_id()
// Get the id of the current thread.
#define thread_id() asm () -> u64 { syscall thread_id; }

// void thread_sleep(u64 time_ms)
// Make the current thread sleep for at least the given time in milliseconds.
#define thread_sleep(__time_ms) asm (__time_ms) -> void { syscall thread_sleep; }

// u64 thread_join(u64 tid)
// Join on the thread with the given id. Produces the return value for the thread.
#define thread_join(__tid) asm (__tid) -> u64 { syscall thread_join; }

// void print_i64(i64 val)
// Print an i64 value to standard output.
#define print_i64(__val) asm (__val) -> void { syscall print_i64; }

// void print_f32(f32 val)
// Print an f32 value to standard output.
#define print_f32(__val) asm (__val) -> void { syscall print_f32; }

// void print_str(const char* str)
// Print a string to standard output.
#define print_str(__str) asm (__str) -> void { syscall print_str; }

// void print_endl()
// Print a newline to standard output.
#define print_endl() asm () -> void { syscall print_endl; }

// i32 putchar(i32 char)
// Write one byte to standard output. This is a blocking function. The value -1 is returned on end of file or error. Otherwise the byte written is returned.
#define putchar(__char) asm (__char) -> i32 { syscall putchar; }

// i32 getchar()
// Read one byte from standard input. This is a blocking function. The value -1 is returned on end of file or error.
#define getchar() asm () -> i32 { syscall getchar; }

// u64 time_current_ms()
// Get the UNIX time stamp in milliseconds.
#define time_current_ms() asm () -> u64 { syscall time_current_ms; }

// void time_delay_cb(u64 delay_ms, void* callback)
// Schedule a callback to be called once after a given delay.
#define time_delay_cb(__delay_ms, __callback) asm (__delay_ms, __callback) -> void { syscall time_delay_cb; }

// u32 window_create(u32 width, u32 height, const char* title, u64 flags)
// Create a new window with a frame buffer to draw into. The window is initially hidden when created, and will appear as soon as the first frame of image data is drawn.
#define window_create(__width, __height, __title, __flags) asm (__width, __height, __title, __flags) -> u32 { syscall window_create; }

// void window_draw_frame(u32 window_id, const u8* pixel_data)
// Copy a frame of pixels to be displayed into the window. The frame must have the same width and height as the window. The pixel format is 32 bits per pixel in BGRA byte order, with 8 bits for each component and the B byte at the lowest address.
#define window_draw_frame(__window_id, __pixel_data) asm (__window_id, __pixel_data) -> void { syscall window_draw_frame; }

// bool window_poll_event(void* p_event)
// Try to read an event from the windowing system if available. The event is read into an event struct. Boolean true is returned if an event was read, false if not.
#define window_poll_event(__p_event) asm (__p_event) -> bool { syscall window_poll_event; }

// u32 audio_open_output(u32 sample_rate, u16 num_channels, u16 format, void* callback)
// Open an audio output device.
#define audio_open_output(__sample_rate, __num_channels, __format, __callback) asm (__sample_rate, __num_channels, __format, __callback) -> u32 { syscall audio_open_output; }

// u64 net_listen(const char* listen_addr, void* on_new_conn)
// Open a listening TCP socket to accept incoming connections. A callback function is called when a new connection request is received.
#define net_listen(__listen_addr, __on_new_conn) asm (__listen_addr, __on_new_conn) -> u64 { syscall net_listen; }

// u64 net_accept(u64 socket_id, char* client_addr_buf, u64 addr_buf_len, void* on_incoming_data)
// Accept an incoming connection and creates a new socket. A callback function is called when incoming data is received on the new socket.
#define net_accept(__socket_id, __client_addr_buf, __addr_buf_len, __on_incoming_data) asm (__socket_id, __client_addr_buf, __addr_buf_len, __on_incoming_data) -> u64 { syscall net_accept; }

// u64 net_read(u64 socket_id, u8* buf_ptr, u64 buf_len)
// Read data from a socket into a buffer with specified capacity. Data can only be read if available.
#define net_read(__socket_id, __buf_ptr, __buf_len) asm (__socket_id, __buf_ptr, __buf_len) -> u64 { syscall net_read; }

// u64 net_write(u64 socket_id, const u8* buf_ptr, u64 buf_len)
// Write data to an open socket. This function will attempt to write the entire buffer and may block if the output buffer is full.
#define net_write(__socket_id, __buf_ptr, __buf_len) asm (__socket_id, __buf_ptr, __buf_len) -> u64 { syscall net_write; }

// void net_close(u64 socket_id)
// Close an open socket.
#define net_close(__socket_id) asm (__socket_id) -> void { syscall net_close; }

#define EVENT_QUIT 0
#define EVENT_KEYDOWN 1
#define EVENT_KEYUP 2
#define EVENT_MOUSEDOWN 3
#define EVENT_MOUSEUP 4
#define EVENT_MOUSEMOVE 5
#define KEY_BACKSPACE 8
#define KEY_TAB 9
#define KEY_RETURN 10
#define KEY_ESCAPE 27
#define KEY_SPACE 32
#define KEY_COMMA 44
#define KEY_PERIOD 46
#define KEY_SLASH 47
#define KEY_NUM0 48
#define KEY_NUM1 49
#define KEY_NUM2 50
#define KEY_NUM3 51
#define KEY_NUM4 52
#define KEY_NUM5 53
#define KEY_NUM6 54
#define KEY_NUM7 55
#define KEY_NUM8 56
#define KEY_NUM9 57
#define KEY_COLON 58
#define KEY_SEMICOLON 59
#define KEY_EQUALS 61
#define KEY_QUESTION 63
#define KEY_A 65
#define KEY_B 66
#define KEY_C 67
#define KEY_D 68
#define KEY_E 69
#define KEY_F 70
#define KEY_G 71
#define KEY_H 72
#define KEY_I 73
#define KEY_J 74
#define KEY_K 75
#define KEY_L 76
#define KEY_M 77
#define KEY_N 78
#define KEY_O 79
#define KEY_P 80
#define KEY_Q 81
#define KEY_R 82
#define KEY_S 83
#define KEY_T 84
#define KEY_U 85
#define KEY_V 86
#define KEY_W 87
#define KEY_X 88
#define KEY_Y 89
#define KEY_Z 90
#define KEY_LEFT 16001
#define KEY_RIGHT 16002
#define KEY_UP 16003
#define KEY_DOWN 16004
#define KEY_SHIFT 16005
#define AUDIO_FORMAT_I16 0

#endif
