# UVM Subsystems and System Calls

This file was automatically generated from [spec/syscalls.json](/spec/syscalls.json).

The host APIs exposed to programs running on UVM are organized into
multiple subsystems described in this document.
Each subsystem includes a number of system calls (syscalls).
Arguments to syscalls are pushed on the stack in order.
Each syscall has fixed arity, that is, the number of input arguments is fixed,
and can output either 0 or 1 value on the stack.
The syscalls with a `void` return type do not output anything.

# vm

Core functionality provided by the VM that isn't related to any kind of I/O.

## memset

```
void memset(void* dst, u8 value, u64 num_bytes)
```

Fill a block of bytes in the heap with a given value.

## memset32

```
void memset32(u32* dst, u32 word, u64 num_words)
```

Fill a region of memory with 32-bit values. This is useful for some graphics operations.

## memcpy

```
void memcpy(void* dst, const void* src, u64 num_bytes)
```

Copy a block of memory in the heap from a source address to a destination address.

## memcmp

```
i32 memcmp(const void* p_a, const void* p_b, u64 num_bytes)
```

**Returns:** `i32 result`

Compare two sequences of bytes. Returns 0 if equal, -1 if the first mismatching byte has a lower value in `p_a`, 1 if greater.

## vm_heap_size

```
u64 vm_heap_size()
```

**Returns:** `u64 num_bytes`

Report the current heap size in bytes.

## vm_grow_heap

```
u64 vm_grow_heap(u64 num_bytes)
```

**Returns:** `u64 new_size`

Grow the heap to a new size given in bytes. This is similar to the `brk()` system call on POSIX systems. Note that the heap may be resized to a size larger than requested. The heap size is guaranteed to be a multiple of 8 bytes. If the requested size is smaller than the current heap size, this is a no-op. Returns the new heap size in bytes.

## exit

```
void exit(i8 status)
```

End program execution with the specified exit status.

## thread_spawn

```
u64 thread_spawn(void* fptr, void* arg)
```

**Returns:** `u64 tid`

Spawn a new thread running the given function with the argument value `arg`. This is a low-level primitive: the spawned thread has no software (alloca) stack, so a C thread function that uses local arrays, address-taken locals, or alloca will fault. From C, prefer pthread_create() from <pthread.h>, which installs a private stack for the new thread.

## thread_id

```
u64 thread_id()
```

**Returns:** `u64 tid`

Get the id of the current thread.

## thread_sleep

```
void thread_sleep(u64 time_ms)
```

Make the current thread sleep for at least the given time in milliseconds.

## thread_join

```
u64 thread_join(u64 tid)
```

**Returns:** `u64 val`

Join on the thread with the given id (as returned by thread_spawn). Produces the return value for the thread. From C, prefer pthread_join() from <pthread.h> when the thread was created with pthread_create().

## cmd_argc

```
u64 cmd_argc()
```

**Returns:** `u64 argc`

Get the number of command-line arguments passed to the program. Argument 0 is the program file path.

## cmd_get_arg

```
u64 cmd_get_arg(u64 idx, u8* dst, u64 dst_len)
```

**Returns:** `u64 len`

Copy command-line argument idx into the buffer dst with capacity dst_len bytes. The copy is NUL-terminated whenever dst_len is at least 1, and truncated to fit. Returns the full byte length of the argument excluding the NUL terminator, so calling with dst_len = 0 queries the size needed.

# io

Stream I/O functionality.

## print_i64

```
void print_i64(i64 val)
```

Print an i64 value to standard output.

## print_f32

```
void print_f32(f32 val)
```

Print an f32 value to standard output.

## print_str

```
void print_str(const char* str)
```

Print a string to standard output.

## print_endl

```
void print_endl()
```

Print a newline to standard output.

## putchar

```
i32 putchar(i32 char)
```

**Returns:** `i32 char`

Write one byte to standard output. This is a blocking function. The value -1 is returned on end of file or error. Otherwise the byte written is returned.

## getchar

```
i32 getchar()
```

**Returns:** `i32 val`

Read one byte from standard input. This is a blocking function. The value -1 is returned on end of file or error.

# time

Date, time and timing related system calls.

## time_current_ms

```
u64 time_current_ms()
```

**Returns:** `u64 timestamp`

Get the UNIX time stamp in milliseconds.

# window

Functionality related to creating windows, drawing graphics, as well as mouse and keyboard input.

## window_create

```
u32 window_create(u32 width, u32 height, const char* title, u64 flags)
```

**Returns:** `u32 window_id`

Create a new window with a frame buffer to draw into. The window is initially hidden when created, and will appear as soon as the first frame of image data is drawn.

## window_draw_frame

```
void window_draw_frame(u32 window_id, const u8* pixel_data)
```

Copy a frame of pixels to be displayed into the window. The frame must have the same width and height as the window. The pixel format is 32 bits per pixel in BGRA byte order, with 8 bits for each component and the B byte at the lowest address.

## window_poll_event

```
bool window_poll_event(void* p_event)
```

**Returns:** `bool event_read`

Try to read an event from the windowing system if available. The event is read into an event struct. Boolean true is returned if an event was read, false if not.

## window_wait_event

```
void window_wait_event(void* p_event)
```

Block until an window event is available.

## Constants
These are the constants associated with the window subsystem:

- `u16 EVENT_QUIT = 0`
- `u16 EVENT_KEYDOWN = 1`
- `u16 EVENT_KEYUP = 2`
- `u16 EVENT_MOUSEDOWN = 3`
- `u16 EVENT_MOUSEUP = 4`
- `u16 EVENT_MOUSEMOVE = 5`
- `u16 EVENT_TEXTINPUT = 6`
- `u16 KEY_BACKSPACE = 8`
- `u16 KEY_TAB = 9`
- `u16 KEY_RETURN = 10`
- `u16 KEY_ESCAPE = 27`
- `u16 KEY_SPACE = 32`
- `u16 KEY_COMMA = 44`
- `u16 KEY_PERIOD = 46`
- `u16 KEY_SLASH = 47`
- `u16 KEY_NUM0 = 48`
- `u16 KEY_NUM1 = 49`
- `u16 KEY_NUM2 = 50`
- `u16 KEY_NUM3 = 51`
- `u16 KEY_NUM4 = 52`
- `u16 KEY_NUM5 = 53`
- `u16 KEY_NUM6 = 54`
- `u16 KEY_NUM7 = 55`
- `u16 KEY_NUM8 = 56`
- `u16 KEY_NUM9 = 57`
- `u16 KEY_COLON = 58`
- `u16 KEY_SEMICOLON = 59`
- `u16 KEY_EQUALS = 61`
- `u16 KEY_QUESTION = 63`
- `u16 KEY_A = 65`
- `u16 KEY_B = 66`
- `u16 KEY_C = 67`
- `u16 KEY_D = 68`
- `u16 KEY_E = 69`
- `u16 KEY_F = 70`
- `u16 KEY_G = 71`
- `u16 KEY_H = 72`
- `u16 KEY_I = 73`
- `u16 KEY_J = 74`
- `u16 KEY_K = 75`
- `u16 KEY_L = 76`
- `u16 KEY_M = 77`
- `u16 KEY_N = 78`
- `u16 KEY_O = 79`
- `u16 KEY_P = 80`
- `u16 KEY_Q = 81`
- `u16 KEY_R = 82`
- `u16 KEY_S = 83`
- `u16 KEY_T = 84`
- `u16 KEY_U = 85`
- `u16 KEY_V = 86`
- `u16 KEY_W = 87`
- `u16 KEY_X = 88`
- `u16 KEY_Y = 89`
- `u16 KEY_Z = 90`
- `u16 KEY_LEFT = 16001`
- `u16 KEY_RIGHT = 16002`
- `u16 KEY_UP = 16003`
- `u16 KEY_DOWN = 16004`
- `u16 KEY_SHIFT = 16005`

# audio

Audio input and output.

## audio_open_output

```
u32 audio_open_output(u32 sample_rate, u16 num_channels, u16 format)
```

**Returns:** `u32 device_id`

Open an audio output device for blocking playback. Returns a device id used with audio_wait_output / audio_write / audio_close. The device runs on the caller's own thread(s): no callback thread is spawned.

## audio_wait_output

```
void audio_wait_output(u32 device_id)
```

Block until the output device needs the next buffer of samples, then return. Call audio_write next to hand it the samples. This just-in-time rendezvous keeps output latency to about one buffer period.

## audio_write

```
void audio_write(u32 device_id, i16* samples, u32 num_frames)
```

Submit num_frames of interleaved samples (num_frames * num_channels i16 values) to the output device, copying them into the device buffer. Intended to follow audio_wait_output.

## audio_open_input

```
u32 audio_open_input(u32 sample_rate, u16 num_channels, u16 format)
```

**Returns:** `u32 device_id`

Open an audio input device for blocking capture. Returns a device id used with audio_read / audio_close. The device runs on the caller's own thread(s): no callback thread is spawned.

## audio_read

```
void audio_read(u32 device_id, i16* samples, u32 num_frames)
```

Block until num_frames of interleaved input samples (num_frames * num_channels i16 values) have been captured, then copy them into the provided buffer.

## audio_close

```
void audio_close(u32 device_id)
```

Close an audio device (input or output), stopping it and unblocking any thread waiting in audio_wait_output / audio_read.

## Constants
These are the constants associated with the audio subsystem:

- `u16 AUDIO_FORMAT_I16 = 0`

# net

Network-related functionality.

## net_listen

```
i64 net_listen(const char* listen_addr)
```

**Returns:** `i64 socket_id`

Open a listening TCP socket bound to the given address (e.g. "127.0.0.1:9000"). Returns a socket id (a positive integer) to be passed to net_accept, or -1 on failure.

## net_accept

```
i64 net_accept(u64 socket_id, char* client_addr_buf, u64 addr_buf_len)
```

**Returns:** `i64 socket_id`

Block until an incoming connection is received on a listening socket, then create a new socket for it. The client's address is written into client_addr_buf as a NUL-terminated string, truncated to addr_buf_len. Returns the new connection's socket id (a positive integer), or -1 if the listening socket is closed (by net_close from another thread) or on error.

## net_read

```
i64 net_read(u64 socket_id, u8* buf_ptr, u64 buf_len)
```

**Returns:** `i64 num_bytes`

Read data from a socket into a buffer with the given capacity. Blocks until at least one byte is available (unless a read timeout has been set with net_set_read_timeout). Returns the number of bytes read, 0 when the connection has been closed by the peer, -1 on error, or -2 if the read timed out.

## net_write

```
i64 net_write(u64 socket_id, const u8* buf_ptr, u64 buf_len)
```

**Returns:** `i64 num_bytes`

Write data to an open socket. Blocks until the entire buffer has been written. Returns the number of bytes written, or -1 if the connection was lost.

## net_close

```
i64 net_close(u64 socket_id)
```

**Returns:** `i64 status`

Close an open socket. Closing a listening socket also cancels a thread blocked in net_accept on it; closing a connected socket wakes a thread blocked in net_read on it. Returns 0 on success, or NET_ERROR if the socket id is unknown.

## net_set_read_timeout

```
i64 net_set_read_timeout(u64 socket_id, u64 timeout_ms)
```

**Returns:** `i64 status`

Set the read timeout on a connected socket, in milliseconds. When set, net_read blocks for at most timeout_ms and returns NET_TIMEOUT if no data arrives in that window. A timeout of 0 clears the timeout, making subsequent reads block indefinitely. Returns 0 on success, or NET_ERROR on failure.

## Constants
These are the constants associated with the net subsystem:

- `i64 NET_EOF = 0`
- `i64 NET_ERROR = -1`
- `i64 NET_TIMEOUT = -2`

# fs

File I/O and filesystem-related functionality. This subsystem is separated out from the general-purpose io subsystem for security reasons.

## file_open

```
u64 file_open(const char* path, u64 flags)
```

**Returns:** `u64 handle`

Open the file at the given path. The flags argument is a bitfield combining OPEN_READ, OPEN_WRITE, OPEN_CREATE and OPEN_TRUNC. All access is binary (byte-exact) with no newline translation. Returns a nonzero file handle on success, or 0 on failure (for example if the path is rejected by the sandbox or does not exist).

## file_close

```
void file_close(u64 handle)
```

Close a file handle previously returned by file_open. Has no effect if the handle is not open.

## file_read

```
i64 file_read(u64 handle, u8* buf, u64 num_bytes)
```

**Returns:** `i64 num_bytes`

Read up to num_bytes from the file into the buffer. Returns the number of bytes actually read, 0 at end of file, or -1 on error.

## file_write

```
i64 file_write(u64 handle, const u8* buf, u64 num_bytes)
```

**Returns:** `i64 num_bytes`

Write num_bytes from the buffer to the file. Returns the number of bytes actually written, or -1 on error.

## file_seek

```
u64 file_seek(u64 handle, u64 pos)
```

**Returns:** `u64 new_pos`

Seek to an absolute byte offset measured from the start of the file. Returns the new absolute position.

## file_tell

```
u64 file_tell(u64 handle)
```

**Returns:** `u64 pos`

Return the current absolute byte offset, measured from the start of the file.

## file_size

```
u64 file_size(u64 handle)
```

**Returns:** `u64 num_bytes`

Return the total size of the file in bytes. Does not change the current file position.

## Constants
These are the constants associated with the fs subsystem:

- `u64 OPEN_READ = 1`
- `u64 OPEN_WRITE = 2`
- `u64 OPEN_CREATE = 4`
- `u64 OPEN_TRUNC = 8`

