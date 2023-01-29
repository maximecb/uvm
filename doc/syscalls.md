# UVM Subsystems and System Calls

The APIs exposed to software running on UVM are divided into
the following subsystems.

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

File and stream I/O functionality.

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

Read an i64 value from standard input.

# time

Date, time and timing related system calls.

## time_current_ms

```
u64 time_current_ms()
```

Get the UNIX time stamp in milliseconds.

## time_delay_cb

```
void time_delay_cb(u64 delay_ms, void* callback)
```

Schedule a callback to be called once after a given delay.

# window

## window_create

```
u32 window_create(u32 width, u32 height, const char* title, u64 flags)
```

Create a new window with a frame buffer to draw into.

## window_show

```
void window_show(u32 window_id)
```

Show a window, initially not visible when created.

## window_draw_frame

```
void window_draw_frame(u32 window_id, const u8* pixel_data)
```

Copy a frame of RGB24 pixels to be displayed into the window.

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

# net

Network-related functionality.

