//
// This file was automatically generated based on api/syscalls.json
//

#![allow(unused)]

const NUM_SYSCALLS: usize = 14;

const MEMCPY: u16 = 3;
const MEMSET: u16 = 4;
const PRINT_I64: u16 = 5;
const PRINT_STR: u16 = 6;
const PRINT_ENDL: u16 = 7;
const READ_I64: u16 = 8;
const TIME_CURRENT_MS: u16 = 0;
const TIME_DELAY_CB: u16 = 2;
const WINDOW_CREATE: u16 = 1;
const WINDOW_SHOW: u16 = 9;
const WINDOW_DRAW_FRAME: u16 = 10;
const WINDOW_ON_MOUSEMOVE: u16 = 11;
const WINDOW_ON_MOUSEDOWN: u16 = 12;
const WINDOW_ON_MOUSEUP: u16 = 13;

struct SysCallDesc
{
    name: &'static str,
    const_idx: u16,
    argc: usize,
}

const SYSCALL_DESCS: [SysCallDesc; NUM_SYSCALLS] = [
    SysCallDesc { name: "time_current_ms", const_idx: 0, argc: 0 },
    SysCallDesc { name: "window_create", const_idx: 1, argc: 4 },
    SysCallDesc { name: "time_delay_cb", const_idx: 2, argc: 2 },
    SysCallDesc { name: "memcpy", const_idx: 3, argc: 3 },
    SysCallDesc { name: "memset", const_idx: 4, argc: 3 },
    SysCallDesc { name: "print_i64", const_idx: 5, argc: 1 },
    SysCallDesc { name: "print_str", const_idx: 6, argc: 1 },
    SysCallDesc { name: "print_endl", const_idx: 7, argc: 0 },
    SysCallDesc { name: "read_i64", const_idx: 8, argc: 0 },
    SysCallDesc { name: "window_show", const_idx: 9, argc: 1 },
    SysCallDesc { name: "window_draw_frame", const_idx: 10, argc: 1 },
    SysCallDesc { name: "window_on_mousemove", const_idx: 11, argc: 2 },
    SysCallDesc { name: "window_on_mousedown", const_idx: 12, argc: 2 },
    SysCallDesc { name: "window_on_mouseup", const_idx: 13, argc: 2 },
];
