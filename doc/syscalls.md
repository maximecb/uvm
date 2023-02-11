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

## memcpy

```
void memcpy(u8* dst, const u8* src, u64 num_bytes)
```

Copy a block of memory in the heap from a source address to a destination address.

## memset

```
void memset(u8* dst, u8 value, u64 num_bytes)
```

Fill a block of bytes in the heap with a given value.

# io

Stream I/O functionality.

## print_i64

```
void print_i64(i64 val)
```

Print an i64 value to standard output.

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

## read_i64

```
i64 read_i64()
```

**Returns:** `i64 val`

Read an i64 value from standard input.

# time

Date, time and timing related system calls.

## time_current_ms

```
u64 time_current_ms()
```

**Returns:** `u64 timestamp`

Get the UNIX time stamp in milliseconds.

## time_delay_cb

```
void time_delay_cb(u64 delay_ms, void* callback)
```

Schedule a callback to be called once after a given delay.

# window

Functionality related to creating windows, drawing graphics, as well as mouse and keyboard input.

## window_create

```
u32 window_create(u32 width, u32 height, const char* title, u64 flags)
```

**Returns:** `u32 window_id`

Create a new window with a frame buffer to draw into.

## window_draw_frame

```
void window_draw_frame(u32 window_id, const u8* pixel_data)
```

Copy a frame of pixels to be displayed into the window. The frame must have the same width and height as the window. The pixel format is 32 bits per pixel in BGRA byte order, with 8 bits for each component and the B byte at the lowest address.

## window_on_mousemove

```
void window_on_mousemove(u32 window_id, void* callback)
```

Register a callback for mouse movement.

## window_on_mousedown

```
void window_on_mousedown(u32 window_id, void* callback)
```

Register a callback for mouse button press events.

## window_on_mouseup

```
void window_on_mouseup(u32 window_id, void* callback)
```

Register a callback for mouse button release events.

# audio

Audio input and output.

# fs

File I/O and filesystem-related functionality. This subsystem is separated out from the general-purpose io subsystem for security reasons.

# net

Network-related functionality.

