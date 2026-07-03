//
// This file was automatically generated based on spec/syscalls.json
//

#ifndef __UVM_SYSCALLS__
#define __UVM_SYSCALLS__

#ifdef __clang__
// Compiled with clang for the uvclang backend. Each syscall is exposed as an
// external function `__uvm_<name>`, which uvclang recognizes and lowers
// to an inline UVM `syscall <name>` instruction. The function-like macros let
// user code call syscalls by their natural names without colliding with
// clang's builtin declarations (memcpy, putchar, ...).

#include <stdint.h>

// void memset(u8* dst, u8 value, u64 num_bytes)
// Fill a block of bytes in the heap with a given value.
extern void __uvm_memset(uint8_t* __dst, uint8_t __value, uint64_t __num_bytes);
#define memset(__dst, __value, __num_bytes) __uvm_memset(__dst, __value, __num_bytes)

// void memset32(u32* dst, u32 word, u64 num_words)
// Fill a region of memory with 32-bit values. This is useful for some graphics operations.
extern void __uvm_memset32(uint32_t* __dst, uint32_t __word, uint64_t __num_words);
#define memset32(__dst, __word, __num_words) __uvm_memset32(__dst, __word, __num_words)

// void memcpy(u8* dst, const u8* src, u64 num_bytes)
// Copy a block of memory in the heap from a source address to a destination address.
extern void __uvm_memcpy(uint8_t* __dst, const uint8_t* __src, uint64_t __num_bytes);
#define memcpy(__dst, __src, __num_bytes) __uvm_memcpy(__dst, __src, __num_bytes)

// i32 memcmp(const u8* p_a, const u8* p_b, u64 num_bytes)
// Compare two sequences of bytes. Returns 0 if equal, -1 if the first mismatching byte has a lower value in `p_a`, 1 if greater.
extern int32_t __uvm_memcmp(const uint8_t* __p_a, const uint8_t* __p_b, uint64_t __num_bytes);
#define memcmp(__p_a, __p_b, __num_bytes) __uvm_memcmp(__p_a, __p_b, __num_bytes)

// u64 vm_heap_size()
// Report the current heap size in bytes.
extern uint64_t __uvm_vm_heap_size(void);
#define vm_heap_size() __uvm_vm_heap_size()

// u64 vm_grow_heap(u64 num_bytes)
// Grow the heap to a new size given in bytes. This is similar to the `brk()` system call on POSIX systems. Note that the heap may be resized to a size larger than requested. The heap size is guaranteed to be a multiple of 8 bytes. If the requested size is smaller than the current heap size, this is a no-op. Returns the new heap size in bytes.
extern uint64_t __uvm_vm_grow_heap(uint64_t __num_bytes);
#define vm_grow_heap(__num_bytes) __uvm_vm_grow_heap(__num_bytes)

// void exit(i8 status)
// End program execution with the specified exit status.
extern void __uvm_exit(int8_t __status);
#define exit(__status) __uvm_exit(__status)

// u64 thread_spawn(void* fptr, void* arg)
// Spawn a new thread running the given function with the argument value `arg`.
extern uint64_t __uvm_thread_spawn(void* __fptr, void* __arg);
#define thread_spawn(__fptr, __arg) __uvm_thread_spawn(__fptr, __arg)

// u64 thread_id()
// Get the id of the current thread.
extern uint64_t __uvm_thread_id(void);
#define thread_id() __uvm_thread_id()

// void thread_sleep(u64 time_ms)
// Make the current thread sleep for at least the given time in milliseconds.
extern void __uvm_thread_sleep(uint64_t __time_ms);
#define thread_sleep(__time_ms) __uvm_thread_sleep(__time_ms)

// u64 thread_join(u64 tid)
// Join on the thread with the given id. Produces the return value for the thread.
extern uint64_t __uvm_thread_join(uint64_t __tid);
#define thread_join(__tid) __uvm_thread_join(__tid)

// u64 cmd_argc()
// Get the number of command-line arguments passed to the program. Argument 0 is the program file path.
extern uint64_t __uvm_cmd_argc(void);
#define cmd_argc() __uvm_cmd_argc()

// u64 cmd_get_arg(u64 idx, u8* dst, u64 dst_len)
// Copy command-line argument idx into the buffer dst with capacity dst_len bytes. The copy is NUL-terminated whenever dst_len is at least 1, and truncated to fit. Returns the full byte length of the argument excluding the NUL terminator, so calling with dst_len = 0 queries the size needed.
extern uint64_t __uvm_cmd_get_arg(uint64_t __idx, uint8_t* __dst, uint64_t __dst_len);
#define cmd_get_arg(__idx, __dst, __dst_len) __uvm_cmd_get_arg(__idx, __dst, __dst_len)

// void print_i64(i64 val)
// Print an i64 value to standard output.
extern void __uvm_print_i64(int64_t __val);
#define print_i64(__val) __uvm_print_i64(__val)

// void print_f32(f32 val)
// Print an f32 value to standard output.
extern void __uvm_print_f32(float __val);
#define print_f32(__val) __uvm_print_f32(__val)

// void print_str(const char* str)
// Print a string to standard output.
extern void __uvm_print_str(const char* __str);
#define print_str(__str) __uvm_print_str(__str)

// void print_endl()
// Print a newline to standard output.
extern void __uvm_print_endl(void);
#define print_endl() __uvm_print_endl()

// i32 putchar(i32 char)
// Write one byte to standard output. This is a blocking function. The value -1 is returned on end of file or error. Otherwise the byte written is returned.
extern int32_t __uvm_putchar(int32_t __char);
#define putchar(__char) __uvm_putchar(__char)

// i32 getchar()
// Read one byte from standard input. This is a blocking function. The value -1 is returned on end of file or error.
extern int32_t __uvm_getchar(void);
#define getchar() __uvm_getchar()

// u64 time_current_ms()
// Get the UNIX time stamp in milliseconds.
extern uint64_t __uvm_time_current_ms(void);
#define time_current_ms() __uvm_time_current_ms()

// u32 window_create(u32 width, u32 height, const char* title, u64 flags)
// Create a new window with a frame buffer to draw into. The window is initially hidden when created, and will appear as soon as the first frame of image data is drawn.
extern uint32_t __uvm_window_create(uint32_t __width, uint32_t __height, const char* __title, uint64_t __flags);
#define window_create(__width, __height, __title, __flags) __uvm_window_create(__width, __height, __title, __flags)

// void window_draw_frame(u32 window_id, const u8* pixel_data)
// Copy a frame of pixels to be displayed into the window. The frame must have the same width and height as the window. The pixel format is 32 bits per pixel in BGRA byte order, with 8 bits for each component and the B byte at the lowest address.
extern void __uvm_window_draw_frame(uint32_t __window_id, const uint8_t* __pixel_data);
#define window_draw_frame(__window_id, __pixel_data) __uvm_window_draw_frame(__window_id, __pixel_data)

// bool window_poll_event(void* p_event)
// Try to read an event from the windowing system if available. The event is read into an event struct. Boolean true is returned if an event was read, false if not.
extern _Bool __uvm_window_poll_event(void* __p_event);
#define window_poll_event(__p_event) __uvm_window_poll_event(__p_event)

// void window_wait_event(void* p_event)
// Block until an window event is available.
extern void __uvm_window_wait_event(void* __p_event);
#define window_wait_event(__p_event) __uvm_window_wait_event(__p_event)

// u32 audio_open_output(u32 sample_rate, u16 num_channels, u16 format, void* callback)
// Open an audio output device, then spawn a new thread which will regularly call the specified callback function to generate audio samples.
extern uint32_t __uvm_audio_open_output(uint32_t __sample_rate, uint16_t __num_channels, uint16_t __format, void* __callback);
#define audio_open_output(__sample_rate, __num_channels, __format, __callback) __uvm_audio_open_output(__sample_rate, __num_channels, __format, __callback)

// u32 audio_open_input(u32 sample_rate, u16 num_channels, u16 format, void* callback)
// Open an audio input device, then spawn a new thread which will regularly call the specified callback function to process audio samples.
extern uint32_t __uvm_audio_open_input(uint32_t __sample_rate, uint16_t __num_channels, uint16_t __format, void* __callback);
#define audio_open_input(__sample_rate, __num_channels, __format, __callback) __uvm_audio_open_input(__sample_rate, __num_channels, __format, __callback)

// void audio_read_samples(i16* dst_buf, u32 num_samples)
// Read available input samples. Must be called from the audio input thread.
extern void __uvm_audio_read_samples(int16_t* __dst_buf, uint32_t __num_samples);
#define audio_read_samples(__dst_buf, __num_samples) __uvm_audio_read_samples(__dst_buf, __num_samples)

// i64 net_listen(const char* listen_addr)
// Open a listening TCP socket bound to the given address (e.g. "127.0.0.1:9000"). Returns a socket id (a positive integer) to be passed to net_accept, or -1 on failure.
extern int64_t __uvm_net_listen(const char* __listen_addr);
#define net_listen(__listen_addr) __uvm_net_listen(__listen_addr)

// i64 net_accept(u64 socket_id, char* client_addr_buf, u64 addr_buf_len)
// Block until an incoming connection is received on a listening socket, then create a new socket for it. The client's address is written into client_addr_buf as a NUL-terminated string, truncated to addr_buf_len. Returns the new connection's socket id (a positive integer), or -1 if the listening socket is closed (by net_close from another thread) or on error.
extern int64_t __uvm_net_accept(uint64_t __socket_id, char* __client_addr_buf, uint64_t __addr_buf_len);
#define net_accept(__socket_id, __client_addr_buf, __addr_buf_len) __uvm_net_accept(__socket_id, __client_addr_buf, __addr_buf_len)

// i64 net_read(u64 socket_id, u8* buf_ptr, u64 buf_len)
// Read data from a socket into a buffer with the given capacity. Blocks until at least one byte is available (unless a read timeout has been set with net_set_read_timeout). Returns the number of bytes read, 0 when the connection has been closed by the peer, -1 on error, or -2 if the read timed out.
extern int64_t __uvm_net_read(uint64_t __socket_id, uint8_t* __buf_ptr, uint64_t __buf_len);
#define net_read(__socket_id, __buf_ptr, __buf_len) __uvm_net_read(__socket_id, __buf_ptr, __buf_len)

// i64 net_write(u64 socket_id, const u8* buf_ptr, u64 buf_len)
// Write data to an open socket. Blocks until the entire buffer has been written. Returns the number of bytes written, or -1 if the connection was lost.
extern int64_t __uvm_net_write(uint64_t __socket_id, const uint8_t* __buf_ptr, uint64_t __buf_len);
#define net_write(__socket_id, __buf_ptr, __buf_len) __uvm_net_write(__socket_id, __buf_ptr, __buf_len)

// i64 net_close(u64 socket_id)
// Close an open socket. Closing a listening socket also cancels a thread blocked in net_accept on it; closing a connected socket wakes a thread blocked in net_read on it. Returns 0 on success, or NET_ERROR if the socket id is unknown.
extern int64_t __uvm_net_close(uint64_t __socket_id);
#define net_close(__socket_id) __uvm_net_close(__socket_id)

// i64 net_set_read_timeout(u64 socket_id, u64 timeout_ms)
// Set the read timeout on a connected socket, in milliseconds. When set, net_read blocks for at most timeout_ms and returns NET_TIMEOUT if no data arrives in that window. A timeout of 0 clears the timeout, making subsequent reads block indefinitely. Returns 0 on success, or NET_ERROR on failure.
extern int64_t __uvm_net_set_read_timeout(uint64_t __socket_id, uint64_t __timeout_ms);
#define net_set_read_timeout(__socket_id, __timeout_ms) __uvm_net_set_read_timeout(__socket_id, __timeout_ms)

// u64 file_open(const char* path, u64 flags)
// Open the file at the given path. The flags argument is a bitfield combining OPEN_READ, OPEN_WRITE, OPEN_CREATE and OPEN_TRUNC. All access is binary (byte-exact) with no newline translation. Returns a nonzero file handle on success, or 0 on failure (for example if the path is rejected by the sandbox or does not exist).
extern uint64_t __uvm_file_open(const char* __path, uint64_t __flags);
#define file_open(__path, __flags) __uvm_file_open(__path, __flags)

// void file_close(u64 handle)
// Close a file handle previously returned by file_open. Has no effect if the handle is not open.
extern void __uvm_file_close(uint64_t __handle);
#define file_close(__handle) __uvm_file_close(__handle)

// i64 file_read(u64 handle, u8* buf, u64 num_bytes)
// Read up to num_bytes from the file into the buffer. Returns the number of bytes actually read, 0 at end of file, or -1 on error.
extern int64_t __uvm_file_read(uint64_t __handle, uint8_t* __buf, uint64_t __num_bytes);
#define file_read(__handle, __buf, __num_bytes) __uvm_file_read(__handle, __buf, __num_bytes)

// i64 file_write(u64 handle, const u8* buf, u64 num_bytes)
// Write num_bytes from the buffer to the file. Returns the number of bytes actually written, or -1 on error.
extern int64_t __uvm_file_write(uint64_t __handle, const uint8_t* __buf, uint64_t __num_bytes);
#define file_write(__handle, __buf, __num_bytes) __uvm_file_write(__handle, __buf, __num_bytes)

// u64 file_seek(u64 handle, u64 pos)
// Seek to an absolute byte offset measured from the start of the file. Returns the new absolute position.
extern uint64_t __uvm_file_seek(uint64_t __handle, uint64_t __pos);
#define file_seek(__handle, __pos) __uvm_file_seek(__handle, __pos)

// u64 file_tell(u64 handle)
// Return the current absolute byte offset, measured from the start of the file.
extern uint64_t __uvm_file_tell(uint64_t __handle);
#define file_tell(__handle) __uvm_file_tell(__handle)

// u64 file_size(u64 handle)
// Return the total size of the file in bytes. Does not change the current file position.
extern uint64_t __uvm_file_size(uint64_t __handle);
#define file_size(__handle) __uvm_file_size(__handle)

#else
// Compiled with ncc: syscalls expand to inline UVM assembly blocks.

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

// void exit(i8 status)
// End program execution with the specified exit status.
#define exit(__status) asm (__status) -> void { syscall exit; }

// u64 thread_spawn(void* fptr, void* arg)
// Spawn a new thread running the given function with the argument value `arg`.
#define thread_spawn(__fptr, __arg) asm (__fptr, __arg) -> u64 { syscall thread_spawn; }

// u64 thread_id()
// Get the id of the current thread.
#define thread_id() asm () -> u64 { syscall thread_id; }

// void thread_sleep(u64 time_ms)
// Make the current thread sleep for at least the given time in milliseconds.
#define thread_sleep(__time_ms) asm (__time_ms) -> void { syscall thread_sleep; }

// u64 thread_join(u64 tid)
// Join on the thread with the given id. Produces the return value for the thread.
#define thread_join(__tid) asm (__tid) -> u64 { syscall thread_join; }

// u64 cmd_argc()
// Get the number of command-line arguments passed to the program. Argument 0 is the program file path.
#define cmd_argc() asm () -> u64 { syscall cmd_argc; }

// u64 cmd_get_arg(u64 idx, u8* dst, u64 dst_len)
// Copy command-line argument idx into the buffer dst with capacity dst_len bytes. The copy is NUL-terminated whenever dst_len is at least 1, and truncated to fit. Returns the full byte length of the argument excluding the NUL terminator, so calling with dst_len = 0 queries the size needed.
#define cmd_get_arg(__idx, __dst, __dst_len) asm (__idx, __dst, __dst_len) -> u64 { syscall cmd_get_arg; }

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

// u32 window_create(u32 width, u32 height, const char* title, u64 flags)
// Create a new window with a frame buffer to draw into. The window is initially hidden when created, and will appear as soon as the first frame of image data is drawn.
#define window_create(__width, __height, __title, __flags) asm (__width, __height, __title, __flags) -> u32 { syscall window_create; }

// void window_draw_frame(u32 window_id, const u8* pixel_data)
// Copy a frame of pixels to be displayed into the window. The frame must have the same width and height as the window. The pixel format is 32 bits per pixel in BGRA byte order, with 8 bits for each component and the B byte at the lowest address.
#define window_draw_frame(__window_id, __pixel_data) asm (__window_id, __pixel_data) -> void { syscall window_draw_frame; }

// bool window_poll_event(void* p_event)
// Try to read an event from the windowing system if available. The event is read into an event struct. Boolean true is returned if an event was read, false if not.
#define window_poll_event(__p_event) asm (__p_event) -> bool { syscall window_poll_event; }

// void window_wait_event(void* p_event)
// Block until an window event is available.
#define window_wait_event(__p_event) asm (__p_event) -> void { syscall window_wait_event; }

// u32 audio_open_output(u32 sample_rate, u16 num_channels, u16 format, void* callback)
// Open an audio output device, then spawn a new thread which will regularly call the specified callback function to generate audio samples.
#define audio_open_output(__sample_rate, __num_channels, __format, __callback) asm (__sample_rate, __num_channels, __format, __callback) -> u32 { syscall audio_open_output; }

// u32 audio_open_input(u32 sample_rate, u16 num_channels, u16 format, void* callback)
// Open an audio input device, then spawn a new thread which will regularly call the specified callback function to process audio samples.
#define audio_open_input(__sample_rate, __num_channels, __format, __callback) asm (__sample_rate, __num_channels, __format, __callback) -> u32 { syscall audio_open_input; }

// void audio_read_samples(i16* dst_buf, u32 num_samples)
// Read available input samples. Must be called from the audio input thread.
#define audio_read_samples(__dst_buf, __num_samples) asm (__dst_buf, __num_samples) -> void { syscall audio_read_samples; }

// i64 net_listen(const char* listen_addr)
// Open a listening TCP socket bound to the given address (e.g. "127.0.0.1:9000"). Returns a socket id (a positive integer) to be passed to net_accept, or -1 on failure.
#define net_listen(__listen_addr) asm (__listen_addr) -> i64 { syscall net_listen; }

// i64 net_accept(u64 socket_id, char* client_addr_buf, u64 addr_buf_len)
// Block until an incoming connection is received on a listening socket, then create a new socket for it. The client's address is written into client_addr_buf as a NUL-terminated string, truncated to addr_buf_len. Returns the new connection's socket id (a positive integer), or -1 if the listening socket is closed (by net_close from another thread) or on error.
#define net_accept(__socket_id, __client_addr_buf, __addr_buf_len) asm (__socket_id, __client_addr_buf, __addr_buf_len) -> i64 { syscall net_accept; }

// i64 net_read(u64 socket_id, u8* buf_ptr, u64 buf_len)
// Read data from a socket into a buffer with the given capacity. Blocks until at least one byte is available (unless a read timeout has been set with net_set_read_timeout). Returns the number of bytes read, 0 when the connection has been closed by the peer, -1 on error, or -2 if the read timed out.
#define net_read(__socket_id, __buf_ptr, __buf_len) asm (__socket_id, __buf_ptr, __buf_len) -> i64 { syscall net_read; }

// i64 net_write(u64 socket_id, const u8* buf_ptr, u64 buf_len)
// Write data to an open socket. Blocks until the entire buffer has been written. Returns the number of bytes written, or -1 if the connection was lost.
#define net_write(__socket_id, __buf_ptr, __buf_len) asm (__socket_id, __buf_ptr, __buf_len) -> i64 { syscall net_write; }

// i64 net_close(u64 socket_id)
// Close an open socket. Closing a listening socket also cancels a thread blocked in net_accept on it; closing a connected socket wakes a thread blocked in net_read on it. Returns 0 on success, or NET_ERROR if the socket id is unknown.
#define net_close(__socket_id) asm (__socket_id) -> i64 { syscall net_close; }

// i64 net_set_read_timeout(u64 socket_id, u64 timeout_ms)
// Set the read timeout on a connected socket, in milliseconds. When set, net_read blocks for at most timeout_ms and returns NET_TIMEOUT if no data arrives in that window. A timeout of 0 clears the timeout, making subsequent reads block indefinitely. Returns 0 on success, or NET_ERROR on failure.
#define net_set_read_timeout(__socket_id, __timeout_ms) asm (__socket_id, __timeout_ms) -> i64 { syscall net_set_read_timeout; }

// u64 file_open(const char* path, u64 flags)
// Open the file at the given path. The flags argument is a bitfield combining OPEN_READ, OPEN_WRITE, OPEN_CREATE and OPEN_TRUNC. All access is binary (byte-exact) with no newline translation. Returns a nonzero file handle on success, or 0 on failure (for example if the path is rejected by the sandbox or does not exist).
#define file_open(__path, __flags) asm (__path, __flags) -> u64 { syscall file_open; }

// void file_close(u64 handle)
// Close a file handle previously returned by file_open. Has no effect if the handle is not open.
#define file_close(__handle) asm (__handle) -> void { syscall file_close; }

// i64 file_read(u64 handle, u8* buf, u64 num_bytes)
// Read up to num_bytes from the file into the buffer. Returns the number of bytes actually read, 0 at end of file, or -1 on error.
#define file_read(__handle, __buf, __num_bytes) asm (__handle, __buf, __num_bytes) -> i64 { syscall file_read; }

// i64 file_write(u64 handle, const u8* buf, u64 num_bytes)
// Write num_bytes from the buffer to the file. Returns the number of bytes actually written, or -1 on error.
#define file_write(__handle, __buf, __num_bytes) asm (__handle, __buf, __num_bytes) -> i64 { syscall file_write; }

// u64 file_seek(u64 handle, u64 pos)
// Seek to an absolute byte offset measured from the start of the file. Returns the new absolute position.
#define file_seek(__handle, __pos) asm (__handle, __pos) -> u64 { syscall file_seek; }

// u64 file_tell(u64 handle)
// Return the current absolute byte offset, measured from the start of the file.
#define file_tell(__handle) asm (__handle) -> u64 { syscall file_tell; }

// u64 file_size(u64 handle)
// Return the total size of the file in bytes. Does not change the current file position.
#define file_size(__handle) asm (__handle) -> u64 { syscall file_size; }

#endif // __clang__

#define EVENT_QUIT 0
#define EVENT_KEYDOWN 1
#define EVENT_KEYUP 2
#define EVENT_MOUSEDOWN 3
#define EVENT_MOUSEUP 4
#define EVENT_MOUSEMOVE 5
#define EVENT_TEXTINPUT 6
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
#define NET_EOF 0
#define NET_ERROR -1
#define NET_TIMEOUT -2
#define OPEN_READ 1
#define OPEN_WRITE 2
#define OPEN_CREATE 4
#define OPEN_TRUNC 8

#endif
