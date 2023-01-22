# UVM Subsystems and System Calls

# vm

Core functionality provided by the VM that isn't related to any kind of I/O.

## memcpy

Copy a block of memory in the heap from a source address to a destination address.

## memset

Fill a block of bytes in the heap with a given value.

# io

File and stream I/O functionality

## print_i64

Print an i64 value to standard output

## print_str

Print a string to standard output

## print_endl

Print a newline to standard output

## read_i64

Read an i64 value from standard input

# time

Date, time and timing related system calls.

## time_current_ms

Get the UNIX time stamp in milliseconds.

## time_delay_cb

Schedule a callback to be called once after a given delay.

# window

## window_create

Create a new window with a frame buffer to draw into.

## window_show

Show a window, initially not visible when created.

## window_draw_frame

Copy a frame of RGB24 pixels to be displayed into the window.

## window_on_mousemove

Register a callback for mouse movement.

## window_on_mousedown

Register a callback for mouse button press events.

## window_on_mouseup

Register a callback for mouse button release events.

# audio

Audio input and output.

# net

Network-related functionality.

