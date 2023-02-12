//
// This file was automatically generated based on api/syscalls.json
//

#![allow(unused)]

pub const SYSCALL_TBL_LEN: usize = 16;

pub const TIME_CURRENT_MS: u16 = 0;
pub const WINDOW_CREATE: u16 = 1;
pub const TIME_DELAY_CB: u16 = 2;
pub const MEMCPY: u16 = 3;
pub const MEMSET: u16 = 4;
pub const PRINT_I64: u16 = 5;
pub const PRINT_STR: u16 = 6;
pub const PRINT_ENDL: u16 = 7;
pub const READ_I64: u16 = 8;
pub const WINDOW_ON_KEYDOWN: u16 = 9;
pub const WINDOW_DRAW_FRAME: u16 = 10;
pub const WINDOW_ON_MOUSEMOVE: u16 = 11;
pub const WINDOW_ON_MOUSEDOWN: u16 = 12;
pub const WINDOW_ON_MOUSEUP: u16 = 13;
pub const WINDOW_ON_KEYUP: u16 = 15;

pub struct SysCallDesc
{
    pub name: &'static str,
    pub const_idx: u16,
    pub argc: usize,
    pub has_ret: bool,
}

pub const SYSCALL_DESCS: [Option<SysCallDesc>; SYSCALL_TBL_LEN] = [
    Some(SysCallDesc { name: "time_current_ms", const_idx: 0, argc: 0, has_ret: true }),
    Some(SysCallDesc { name: "window_create", const_idx: 1, argc: 4, has_ret: true }),
    Some(SysCallDesc { name: "time_delay_cb", const_idx: 2, argc: 2, has_ret: false }),
    Some(SysCallDesc { name: "memcpy", const_idx: 3, argc: 3, has_ret: false }),
    Some(SysCallDesc { name: "memset", const_idx: 4, argc: 3, has_ret: false }),
    Some(SysCallDesc { name: "print_i64", const_idx: 5, argc: 1, has_ret: false }),
    Some(SysCallDesc { name: "print_str", const_idx: 6, argc: 1, has_ret: false }),
    Some(SysCallDesc { name: "print_endl", const_idx: 7, argc: 0, has_ret: false }),
    Some(SysCallDesc { name: "read_i64", const_idx: 8, argc: 0, has_ret: true }),
    Some(SysCallDesc { name: "window_on_keydown", const_idx: 9, argc: 2, has_ret: false }),
    Some(SysCallDesc { name: "window_draw_frame", const_idx: 10, argc: 2, has_ret: false }),
    Some(SysCallDesc { name: "window_on_mousemove", const_idx: 11, argc: 2, has_ret: false }),
    Some(SysCallDesc { name: "window_on_mousedown", const_idx: 12, argc: 2, has_ret: false }),
    Some(SysCallDesc { name: "window_on_mouseup", const_idx: 13, argc: 2, has_ret: false }),
    None,
    Some(SysCallDesc { name: "window_on_keyup", const_idx: 15, argc: 2, has_ret: false }),
];

pub const KEY_BACKSPACE: u16 = 8;
pub const KEY_TAB: u16 = 9;
pub const KEY_RETURN: u16 = 10;
pub const KEY_ESCAPE: u16 = 27;
pub const KEY_SPACE: u16 = 32;
pub const KEY_NUM0: u16 = 48;
pub const KEY_A: u16 = 65;
pub const KEY_B: u16 = 66;
pub const KEY_C: u16 = 67;
pub const KEY_D: u16 = 68;
pub const KEY_S: u16 = 83;
pub const KEY_W: u16 = 87;
pub const KEY_LEFT: u16 = 16001;
pub const KEY_RIGHT: u16 = 16002;
pub const KEY_UP: u16 = 16003;
pub const KEY_DOWN: u16 = 16004;
pub const KEY_SHIFT: u16 = 16005;
