//
// This file was automatically generated based on api/syscalls.json
//

#![allow(unused)]

pub const SYSCALL_TBL_LEN: usize = 32;

pub const TIME_CURRENT_MS: u16 = 0;
pub const WINDOW_CREATE: u16 = 1;
pub const TIME_DELAY_CB: u16 = 2;
pub const MEMCPY: u16 = 3;
pub const MEMSET: u16 = 4;
pub const PRINT_I64: u16 = 5;
pub const PRINT_STR: u16 = 6;
pub const PRINT_ENDL: u16 = 7;
pub const GETCHAR: u16 = 8;
pub const WINDOW_POLL_EVENT: u16 = 9;
pub const WINDOW_DRAW_FRAME: u16 = 10;
pub const VM_HEAP_SIZE: u16 = 14;
pub const MEMSET32: u16 = 16;
pub const VM_GROW_HEAP: u16 = 17;
pub const AUDIO_OPEN_OUTPUT: u16 = 18;
pub const PRINT_F32: u16 = 20;
pub const NET_LISTEN: u16 = 21;
pub const NET_ACCEPT: u16 = 22;
pub const NET_READ: u16 = 23;
pub const NET_WRITE: u16 = 24;
pub const NET_CLOSE: u16 = 25;
pub const PUTCHAR: u16 = 26;
pub const MEMCMP: u16 = 27;
pub const THREAD_ID: u16 = 28;
pub const THREAD_SPAWN: u16 = 29;
pub const THREAD_SLEEP: u16 = 30;
pub const THREAD_JOIN: u16 = 31;

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
    Some(SysCallDesc { name: "getchar", const_idx: 8, argc: 0, has_ret: true }),
    Some(SysCallDesc { name: "window_poll_event", const_idx: 9, argc: 1, has_ret: true }),
    Some(SysCallDesc { name: "window_draw_frame", const_idx: 10, argc: 2, has_ret: false }),
    None,
    None,
    None,
    Some(SysCallDesc { name: "vm_heap_size", const_idx: 14, argc: 0, has_ret: true }),
    None,
    Some(SysCallDesc { name: "memset32", const_idx: 16, argc: 3, has_ret: false }),
    Some(SysCallDesc { name: "vm_grow_heap", const_idx: 17, argc: 1, has_ret: true }),
    Some(SysCallDesc { name: "audio_open_output", const_idx: 18, argc: 4, has_ret: true }),
    None,
    Some(SysCallDesc { name: "print_f32", const_idx: 20, argc: 1, has_ret: false }),
    Some(SysCallDesc { name: "net_listen", const_idx: 21, argc: 2, has_ret: true }),
    Some(SysCallDesc { name: "net_accept", const_idx: 22, argc: 4, has_ret: true }),
    Some(SysCallDesc { name: "net_read", const_idx: 23, argc: 3, has_ret: true }),
    Some(SysCallDesc { name: "net_write", const_idx: 24, argc: 3, has_ret: true }),
    Some(SysCallDesc { name: "net_close", const_idx: 25, argc: 1, has_ret: false }),
    Some(SysCallDesc { name: "putchar", const_idx: 26, argc: 1, has_ret: true }),
    Some(SysCallDesc { name: "memcmp", const_idx: 27, argc: 3, has_ret: true }),
    Some(SysCallDesc { name: "thread_id", const_idx: 28, argc: 0, has_ret: true }),
    Some(SysCallDesc { name: "thread_spawn", const_idx: 29, argc: 1, has_ret: true }),
    Some(SysCallDesc { name: "thread_sleep", const_idx: 30, argc: 1, has_ret: false }),
    Some(SysCallDesc { name: "thread_join", const_idx: 31, argc: 1, has_ret: true }),
];

pub const EVENT_QUIT: u16 = 0;
pub const EVENT_KEYDOWN: u16 = 1;
pub const EVENT_KEYUP: u16 = 2;
pub const KEY_BACKSPACE: u16 = 8;
pub const KEY_TAB: u16 = 9;
pub const KEY_RETURN: u16 = 10;
pub const KEY_ESCAPE: u16 = 27;
pub const KEY_SPACE: u16 = 32;
pub const KEY_COMMA: u16 = 44;
pub const KEY_PERIOD: u16 = 46;
pub const KEY_SLASH: u16 = 47;
pub const KEY_NUM0: u16 = 48;
pub const KEY_NUM1: u16 = 49;
pub const KEY_NUM2: u16 = 50;
pub const KEY_NUM3: u16 = 51;
pub const KEY_NUM4: u16 = 52;
pub const KEY_NUM5: u16 = 53;
pub const KEY_NUM6: u16 = 54;
pub const KEY_NUM7: u16 = 55;
pub const KEY_NUM8: u16 = 56;
pub const KEY_NUM9: u16 = 57;
pub const KEY_COLON: u16 = 58;
pub const KEY_SEMICOLON: u16 = 59;
pub const KEY_EQUALS: u16 = 61;
pub const KEY_QUESTION: u16 = 63;
pub const KEY_A: u16 = 65;
pub const KEY_B: u16 = 66;
pub const KEY_C: u16 = 67;
pub const KEY_D: u16 = 68;
pub const KEY_E: u16 = 69;
pub const KEY_F: u16 = 70;
pub const KEY_G: u16 = 71;
pub const KEY_H: u16 = 72;
pub const KEY_I: u16 = 73;
pub const KEY_J: u16 = 74;
pub const KEY_K: u16 = 75;
pub const KEY_L: u16 = 76;
pub const KEY_M: u16 = 77;
pub const KEY_N: u16 = 78;
pub const KEY_O: u16 = 79;
pub const KEY_P: u16 = 80;
pub const KEY_Q: u16 = 81;
pub const KEY_R: u16 = 82;
pub const KEY_S: u16 = 83;
pub const KEY_T: u16 = 84;
pub const KEY_U: u16 = 85;
pub const KEY_V: u16 = 86;
pub const KEY_W: u16 = 87;
pub const KEY_X: u16 = 88;
pub const KEY_Y: u16 = 89;
pub const KEY_Z: u16 = 90;
pub const KEY_LEFT: u16 = 16001;
pub const KEY_RIGHT: u16 = 16002;
pub const KEY_UP: u16 = 16003;
pub const KEY_DOWN: u16 = 16004;
pub const KEY_SHIFT: u16 = 16005;
pub const AUDIO_FORMAT_I16: u16 = 0;
