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

## memset32

```
void memset32(u32* dst, u32 word, u64 num_words)
```

Fill a region of memory with 32-bit values. This is useful for some graphics operations.

## vm_heap_size

```
u64 vm_heap_size()
```

**Returns:** `u64 num_bytes`

Report the current heap size in bytes.

## vm_resize_heap

```
void vm_resize_heap(u64 num_bytes)
```

Resize the heap to a new size given in bytes.

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

Create a new window with a frame buffer to draw into. The window is initially hidden when created, and will appear as soon as the first frame of image data is drawn.

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

## window_on_keydown

```
void window_on_keydown(u32 window_id, void* callback)
```

Register a callback for key press event.

## window_on_keyup

```
void window_on_keyup(u32 window_id, void* callback)
```

Register a callback for key release event.

## Constants
These are the constants associated with the window subsystem:

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

# fs

File I/O and filesystem-related functionality. This subsystem is separated out from the general-purpose io subsystem for security reasons.

# net

Network-related functionality.

