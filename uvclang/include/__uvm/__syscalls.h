//
// This file was automatically generated based on spec/syscalls.json
//

#ifndef __UVM___SYSCALLS__
#define __UVM___SYSCALLS__

// Compiled with clang for the uvclang backend. Each syscall is exposed as an
// external function `__uvm_<name>`, which uvclang recognizes and lowers
// to an inline UVM `syscall <name>` instruction.

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
// void memset(void* dst, u8 value, u64 num_bytes)
// Fill a block of bytes in the heap with a given value.
extern void __uvm_memset(void* __dst, uint8_t __value, uint64_t __num_bytes);
// void memset32(u32* dst, u32 word, u64 num_words)
// Fill a region of memory with 32-bit values. This is useful for some graphics operations.
extern void __uvm_memset32(uint32_t* __dst, uint32_t __word, uint64_t __num_words);
// void memcpy(void* dst, const void* src, u64 num_bytes)
// Copy a block of memory in the heap from a source address to a destination address.
extern void __uvm_memcpy(void* __dst, const void* __src, uint64_t __num_bytes);
// i32 memcmp(const void* p_a, const void* p_b, u64 num_bytes)
// Compare two sequences of bytes. Returns 0 if equal, -1 if the first mismatching byte has a lower value in `p_a`, 1 if greater.
extern int32_t __uvm_memcmp(const void* __p_a, const void* __p_b, uint64_t __num_bytes);
// u64 vm_heap_size()
// Report the current heap size in bytes.
extern uint64_t __uvm_vm_heap_size(void);
// u64 vm_grow_heap(u64 num_bytes)
// Grow the heap to a new size given in bytes. This is similar to the `brk()` system call on POSIX systems. Note that the heap may be resized to a size larger than requested. The heap size is guaranteed to be a multiple of 8 bytes. If the requested size is smaller than the current heap size, this is a no-op. Returns the new heap size in bytes.
extern uint64_t __uvm_vm_grow_heap(uint64_t __num_bytes);
// void exit(i8 status)
// End program execution with the specified exit status.
extern void __uvm_exit(int8_t __status);
// u64 thread_spawn(void* fptr, void* arg)
// Spawn a new thread running the given function with the argument value `arg`. This is a low-level primitive: the spawned thread has no software (alloca) stack, so a C thread function that uses local arrays, address-taken locals, or alloca will fault. From C, prefer pthread_create() from <pthread.h>, which installs a private stack for the new thread.
extern uint64_t __uvm_thread_spawn(void* __fptr, void* __arg);
// u64 thread_id()
// Get the id of the current thread.
extern uint64_t __uvm_thread_id(void);
// void thread_sleep(u64 time_ms)
// Make the current thread sleep for at least the given time in milliseconds.
extern void __uvm_thread_sleep(uint64_t __time_ms);
// u64 thread_join(u64 tid)
// Join on the thread with the given id (as returned by thread_spawn). Produces the return value for the thread. From C, prefer pthread_join() from <pthread.h> when the thread was created with pthread_create().
extern uint64_t __uvm_thread_join(uint64_t __tid);
// u64 cmd_argc()
// Get the number of command-line arguments passed to the program. Argument 0 is the program file path.
extern uint64_t __uvm_cmd_argc(void);
// u64 cmd_get_arg(u64 idx, u8* dst, u64 dst_len)
// Copy command-line argument idx into the buffer dst with capacity dst_len bytes. The copy is NUL-terminated whenever dst_len is at least 1, and truncated to fit. Returns the full byte length of the argument excluding the NUL terminator, so calling with dst_len = 0 queries the size needed.
extern uint64_t __uvm_cmd_get_arg(uint64_t __idx, uint8_t* __dst, uint64_t __dst_len);
// void print_i64(i64 val)
// Print an i64 value to standard output.
extern void __uvm_print_i64(int64_t __val);
// void print_f32(f32 val)
// Print an f32 value to standard output.
extern void __uvm_print_f32(float __val);
// void print_str(const char* str)
// Print a string to standard output.
extern void __uvm_print_str(const char* __str);
// void print_endl()
// Print a newline to standard output.
extern void __uvm_print_endl(void);
// i32 putchar(i32 char)
// Write one byte to standard output. This is a blocking function. The value -1 is returned on end of file or error. Otherwise the byte written is returned.
extern int32_t __uvm_putchar(int32_t __char);
// i32 getchar()
// Read one byte from standard input. This is a blocking function. The value -1 is returned on end of file or error.
extern int32_t __uvm_getchar(void);
// u64 time_current_ms()
// Get the UNIX time stamp in milliseconds.
extern uint64_t __uvm_time_current_ms(void);
// u32 window_create(u32 width, u32 height, const char* title, u64 flags)
// Create a new window with a frame buffer to draw into. The window is initially hidden when created, and will appear as soon as the first frame of image data is drawn.
extern uint32_t __uvm_window_create(uint32_t __width, uint32_t __height, const char* __title, uint64_t __flags);
// void window_draw_frame(u32 window_id, const u8* pixel_data)
// Copy a frame of pixels to be displayed into the window. The frame must have the same width and height as the window. The pixel format is 32 bits per pixel in BGRA byte order, with 8 bits for each component and the B byte at the lowest address.
extern void __uvm_window_draw_frame(uint32_t __window_id, const uint8_t* __pixel_data);
// bool window_poll_event(void* p_event)
// Try to read an event from the windowing system if available. The event is read into an event struct. Boolean true is returned if an event was read, false if not.
extern _Bool __uvm_window_poll_event(void* __p_event);
// void window_wait_event(void* p_event)
// Block until an window event is available.
extern void __uvm_window_wait_event(void* __p_event);
// u32 audio_open_output(u32 sample_rate, u16 num_channels, u16 format)
// Open an audio output device for blocking playback. Returns a device id used with audio_wait_output / audio_write / audio_close. The device runs on the caller's own thread(s): no callback thread is spawned.
extern uint32_t __uvm_audio_open_output(uint32_t __sample_rate, uint16_t __num_channels, uint16_t __format);
// void audio_wait_output(u32 device_id)
// Block until the output device needs the next buffer of samples, then return. Call audio_write next to hand it the samples. This just-in-time rendezvous keeps output latency to about one buffer period.
extern void __uvm_audio_wait_output(uint32_t __device_id);
// void audio_write(u32 device_id, i16* samples, u32 num_frames)
// Submit num_frames of interleaved samples (num_frames * num_channels i16 values) to the output device, copying them into the device buffer. Intended to follow audio_wait_output.
extern void __uvm_audio_write(uint32_t __device_id, int16_t* __samples, uint32_t __num_frames);
// u32 audio_open_input(u32 sample_rate, u16 num_channels, u16 format)
// Open an audio input device for blocking capture. Returns a device id used with audio_read / audio_close. The device runs on the caller's own thread(s): no callback thread is spawned.
extern uint32_t __uvm_audio_open_input(uint32_t __sample_rate, uint16_t __num_channels, uint16_t __format);
// void audio_read(u32 device_id, i16* samples, u32 num_frames)
// Block until num_frames of interleaved input samples (num_frames * num_channels i16 values) have been captured, then copy them into the provided buffer.
extern void __uvm_audio_read(uint32_t __device_id, int16_t* __samples, uint32_t __num_frames);
// void audio_close(u32 device_id)
// Close an audio device (input or output), stopping it and unblocking any thread waiting in audio_wait_output / audio_read.
extern void __uvm_audio_close(uint32_t __device_id);
// i64 net_listen(const char* listen_addr)
// Open a listening TCP socket bound to the given address (e.g. "127.0.0.1:9000"). Returns a socket id (a positive integer) to be passed to net_accept, or -1 on failure.
extern int64_t __uvm_net_listen(const char* __listen_addr);
// i64 net_accept(u64 socket_id, char* client_addr_buf, u64 addr_buf_len)
// Block until an incoming connection is received on a listening socket, then create a new socket for it. The client's address is written into client_addr_buf as a NUL-terminated string, truncated to addr_buf_len. Returns the new connection's socket id (a positive integer), or -1 if the listening socket is closed (by net_close from another thread) or on error.
extern int64_t __uvm_net_accept(uint64_t __socket_id, char* __client_addr_buf, uint64_t __addr_buf_len);
// i64 net_read(u64 socket_id, u8* buf_ptr, u64 buf_len)
// Read data from a socket into a buffer with the given capacity. Blocks until at least one byte is available (unless a read timeout has been set with net_set_read_timeout). Returns the number of bytes read, 0 when the connection has been closed by the peer, -1 on error, or -2 if the read timed out.
extern int64_t __uvm_net_read(uint64_t __socket_id, uint8_t* __buf_ptr, uint64_t __buf_len);
// i64 net_write(u64 socket_id, const u8* buf_ptr, u64 buf_len)
// Write data to an open socket. Blocks until the entire buffer has been written. Returns the number of bytes written, or -1 if the connection was lost.
extern int64_t __uvm_net_write(uint64_t __socket_id, const uint8_t* __buf_ptr, uint64_t __buf_len);
// i64 net_close(u64 socket_id)
// Close an open socket. Closing a listening socket also cancels a thread blocked in net_accept on it; closing a connected socket wakes a thread blocked in net_read on it. Returns 0 on success, or NET_ERROR if the socket id is unknown.
extern int64_t __uvm_net_close(uint64_t __socket_id);
// i64 net_set_read_timeout(u64 socket_id, u64 timeout_ms)
// Set the read timeout on a connected socket, in milliseconds. When set, net_read blocks for at most timeout_ms and returns NET_TIMEOUT if no data arrives in that window. A timeout of 0 clears the timeout, making subsequent reads block indefinitely. Returns 0 on success, or NET_ERROR on failure.
extern int64_t __uvm_net_set_read_timeout(uint64_t __socket_id, uint64_t __timeout_ms);
// u64 file_open(const char* path, u64 flags)
// Open the file at the given path. The flags argument is a bitfield combining OPEN_READ, OPEN_WRITE, OPEN_CREATE and OPEN_TRUNC. All access is binary (byte-exact) with no newline translation. Returns a nonzero file handle on success, or 0 on failure (for example if the path is rejected by the sandbox or does not exist).
extern uint64_t __uvm_file_open(const char* __path, uint64_t __flags);
// void file_close(u64 handle)
// Close a file handle previously returned by file_open. Has no effect if the handle is not open.
extern void __uvm_file_close(uint64_t __handle);
// i64 file_read(u64 handle, u8* buf, u64 num_bytes)
// Read up to num_bytes from the file into the buffer. Returns the number of bytes actually read, 0 at end of file, or -1 on error.
extern int64_t __uvm_file_read(uint64_t __handle, uint8_t* __buf, uint64_t __num_bytes);
// i64 file_write(u64 handle, const u8* buf, u64 num_bytes)
// Write num_bytes from the buffer to the file. Returns the number of bytes actually written, or -1 on error.
extern int64_t __uvm_file_write(uint64_t __handle, const uint8_t* __buf, uint64_t __num_bytes);
// u64 file_seek(u64 handle, u64 pos)
// Seek to an absolute byte offset measured from the start of the file. Returns the new absolute position.
extern uint64_t __uvm_file_seek(uint64_t __handle, uint64_t __pos);
// u64 file_tell(u64 handle)
// Return the current absolute byte offset, measured from the start of the file.
extern uint64_t __uvm_file_tell(uint64_t __handle);
// u64 file_size(u64 handle)
// Return the total size of the file in bytes. Does not change the current file position.
extern uint64_t __uvm_file_size(uint64_t __handle);
#ifdef __cplusplus
}
#endif
#endif
