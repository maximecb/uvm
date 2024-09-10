# UVM Subsystems and System Calls

This file was automatically generated from [api/syscalls.json](/api/syscalls.json).

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
void memset(u8* dst, u8 value, u64 num_bytes)
```

Fill a block of bytes in the heap with a given value.

## memset32

```
void memset32(u32* dst, u32 word, u64 num_words)
```

Fill a region of memory with 32-bit values. This is useful for some graphics operations.

## memcpy

```
void memcpy(u8* dst, const u8* src, u64 num_bytes)
```

Copy a block of memory in the heap from a source address to a destination address.

## memcmp

```
i32 memcmp(const u8* p_a, const u8* p_b, u64 num_bytes)
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

Spawn a new thread running the given function with the argument value `arg`.

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

Join on the thread with the given id. Produces the return value for the thread.

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
u32 audio_open_output(u32 sample_rate, u16 num_channels, u16 format, void* callback)
```

**Returns:** `u32 device_id`

Open an audio output device, then spawn a new thread which will regularly call the specified callback function to generate audio samples.

## audio_open_input

```
u32 audio_open_input(u32 sample_rate, u16 num_channels, u16 format, void* callback)
```

**Returns:** `u32 device_id`

Open an audio input device, then spawn a new thread which will regularly call the specified callback function to process audio samples.

## Constants
These are the constants associated with the audio subsystem:

- `u16 AUDIO_FORMAT_I16 = 0`

# net

Network-related functionality.

## net_listen

```
u64 net_listen(const char* listen_addr, void* on_new_conn)
```

**Returns:** `u64 socket_id`

Open a listening TCP socket to accept incoming connections. A callback function is called when a new connection request is received.

## net_accept

```
u64 net_accept(u64 socket_id, char* client_addr_buf, u64 addr_buf_len, void* on_incoming_data)
```

**Returns:** `u64 socket_id`

Accept an incoming connection and creates a new socket. A callback function is called when incoming data is received on the new socket.

## net_read

```
u64 net_read(u64 socket_id, u8* buf_ptr, u64 buf_len)
```

**Returns:** `u64 num_bytes`

Read data from a socket into a buffer with specified capacity. Data can only be read if available.

## net_write

```
u64 net_write(u64 socket_id, const u8* buf_ptr, u64 buf_len)
```

**Returns:** `u64 num_bytes`

Write data to an open socket. This function will attempt to write the entire buffer and may block if the output buffer is full.

## net_close

```
void net_close(u64 socket_id)
```

Close an open socket.

# fs

File I/O and filesystem-related functionality. This subsystem is separated out from the general-purpose io subsystem for security reasons.

