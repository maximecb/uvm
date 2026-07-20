//
// This file was automatically generated based on spec/syscalls.json
//

#![allow(unused)]

pub const SYSCALL_TBL_LEN: usize = 43;

pub const TIME_CURRENT_MS: u16 = 0;
pub const WINDOW_CREATE: u16 = 1;
pub const WINDOW_WAIT_EVENT: u16 = 2;
pub const MEMCPY: u16 = 3;
pub const MEMSET: u16 = 4;
pub const PRINT_I64: u16 = 5;
pub const PRINT_STR: u16 = 6;
pub const PRINT_ENDL: u16 = 7;
pub const GETCHAR: u16 = 8;
pub const WINDOW_POLL_EVENT: u16 = 9;
pub const WINDOW_DRAW_FRAME: u16 = 10;
pub const EXIT: u16 = 11;
pub const AUDIO_OPEN_INPUT: u16 = 12;
pub const AUDIO_READ: u16 = 13;
pub const VM_HEAP_SIZE: u16 = 14;
pub const FILE_OPEN: u16 = 15;
pub const MEMSET32: u16 = 16;
pub const VM_GROW_HEAP: u16 = 17;
pub const AUDIO_OPEN_OUTPUT: u16 = 18;
pub const FILE_CLOSE: u16 = 19;
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
pub const FILE_READ: u16 = 32;
pub const FILE_WRITE: u16 = 33;
pub const FILE_SEEK: u16 = 34;
pub const FILE_TELL: u16 = 35;
pub const FILE_SIZE: u16 = 36;
pub const CMD_ARGC: u16 = 37;
pub const CMD_GET_ARG: u16 = 38;
pub const NET_SET_READ_TIMEOUT: u16 = 39;
pub const AUDIO_WAIT_OUTPUT: u16 = 40;
pub const AUDIO_WRITE: u16 = 41;
pub const AUDIO_CLOSE: u16 = 42;

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
    Some(SysCallDesc { name: "window_wait_event", const_idx: 2, argc: 1, has_ret: false }),
    Some(SysCallDesc { name: "memcpy", const_idx: 3, argc: 3, has_ret: false }),
    Some(SysCallDesc { name: "memset", const_idx: 4, argc: 3, has_ret: false }),
    Some(SysCallDesc { name: "print_i64", const_idx: 5, argc: 1, has_ret: false }),
    Some(SysCallDesc { name: "print_str", const_idx: 6, argc: 1, has_ret: false }),
    Some(SysCallDesc { name: "print_endl", const_idx: 7, argc: 0, has_ret: false }),
    Some(SysCallDesc { name: "getchar", const_idx: 8, argc: 0, has_ret: true }),
    Some(SysCallDesc { name: "window_poll_event", const_idx: 9, argc: 1, has_ret: true }),
    Some(SysCallDesc { name: "window_draw_frame", const_idx: 10, argc: 2, has_ret: false }),
    Some(SysCallDesc { name: "exit", const_idx: 11, argc: 1, has_ret: false }),
    Some(SysCallDesc { name: "audio_open_input", const_idx: 12, argc: 3, has_ret: true }),
    Some(SysCallDesc { name: "audio_read", const_idx: 13, argc: 3, has_ret: false }),
    Some(SysCallDesc { name: "vm_heap_size", const_idx: 14, argc: 0, has_ret: true }),
    Some(SysCallDesc { name: "file_open", const_idx: 15, argc: 2, has_ret: true }),
    Some(SysCallDesc { name: "memset32", const_idx: 16, argc: 3, has_ret: false }),
    Some(SysCallDesc { name: "vm_grow_heap", const_idx: 17, argc: 1, has_ret: true }),
    Some(SysCallDesc { name: "audio_open_output", const_idx: 18, argc: 3, has_ret: true }),
    Some(SysCallDesc { name: "file_close", const_idx: 19, argc: 1, has_ret: false }),
    Some(SysCallDesc { name: "print_f32", const_idx: 20, argc: 1, has_ret: false }),
    Some(SysCallDesc { name: "net_listen", const_idx: 21, argc: 1, has_ret: true }),
    Some(SysCallDesc { name: "net_accept", const_idx: 22, argc: 3, has_ret: true }),
    Some(SysCallDesc { name: "net_read", const_idx: 23, argc: 3, has_ret: true }),
    Some(SysCallDesc { name: "net_write", const_idx: 24, argc: 3, has_ret: true }),
    Some(SysCallDesc { name: "net_close", const_idx: 25, argc: 1, has_ret: true }),
    Some(SysCallDesc { name: "putchar", const_idx: 26, argc: 1, has_ret: true }),
    Some(SysCallDesc { name: "memcmp", const_idx: 27, argc: 3, has_ret: true }),
    Some(SysCallDesc { name: "thread_id", const_idx: 28, argc: 0, has_ret: true }),
    Some(SysCallDesc { name: "thread_spawn", const_idx: 29, argc: 2, has_ret: true }),
    Some(SysCallDesc { name: "thread_sleep", const_idx: 30, argc: 1, has_ret: false }),
    Some(SysCallDesc { name: "thread_join", const_idx: 31, argc: 1, has_ret: true }),
    Some(SysCallDesc { name: "file_read", const_idx: 32, argc: 3, has_ret: true }),
    Some(SysCallDesc { name: "file_write", const_idx: 33, argc: 3, has_ret: true }),
    Some(SysCallDesc { name: "file_seek", const_idx: 34, argc: 2, has_ret: true }),
    Some(SysCallDesc { name: "file_tell", const_idx: 35, argc: 1, has_ret: true }),
    Some(SysCallDesc { name: "file_size", const_idx: 36, argc: 1, has_ret: true }),
    Some(SysCallDesc { name: "cmd_argc", const_idx: 37, argc: 0, has_ret: true }),
    Some(SysCallDesc { name: "cmd_get_arg", const_idx: 38, argc: 3, has_ret: true }),
    Some(SysCallDesc { name: "net_set_read_timeout", const_idx: 39, argc: 2, has_ret: true }),
    Some(SysCallDesc { name: "audio_wait_output", const_idx: 40, argc: 1, has_ret: false }),
    Some(SysCallDesc { name: "audio_write", const_idx: 41, argc: 3, has_ret: false }),
    Some(SysCallDesc { name: "audio_close", const_idx: 42, argc: 1, has_ret: false }),
];

pub const EVENT_QUIT: u16 = 0;
pub const EVENT_KEYDOWN: u16 = 1;
pub const EVENT_KEYUP: u16 = 2;
pub const EVENT_MOUSEDOWN: u16 = 3;
pub const EVENT_MOUSEUP: u16 = 4;
pub const EVENT_MOUSEMOVE: u16 = 5;
pub const EVENT_TEXTINPUT: u16 = 6;
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
pub const NET_EOF: i64 = 0;
pub const NET_ERROR: i64 = -1;
pub const NET_TIMEOUT: i64 = -2;
pub const OPEN_READ: u64 = 1;
pub const OPEN_WRITE: u64 = 2;
pub const OPEN_CREATE: u64 = 4;
pub const OPEN_TRUNC: u64 = 8;

pub const CONST_DESCS: [(&'static str, i128); 68] = [
    ("EVENT_QUIT", 0),
    ("EVENT_KEYDOWN", 1),
    ("EVENT_KEYUP", 2),
    ("EVENT_MOUSEDOWN", 3),
    ("EVENT_MOUSEUP", 4),
    ("EVENT_MOUSEMOVE", 5),
    ("EVENT_TEXTINPUT", 6),
    ("KEY_BACKSPACE", 8),
    ("KEY_TAB", 9),
    ("KEY_RETURN", 10),
    ("KEY_ESCAPE", 27),
    ("KEY_SPACE", 32),
    ("KEY_COMMA", 44),
    ("KEY_PERIOD", 46),
    ("KEY_SLASH", 47),
    ("KEY_NUM0", 48),
    ("KEY_NUM1", 49),
    ("KEY_NUM2", 50),
    ("KEY_NUM3", 51),
    ("KEY_NUM4", 52),
    ("KEY_NUM5", 53),
    ("KEY_NUM6", 54),
    ("KEY_NUM7", 55),
    ("KEY_NUM8", 56),
    ("KEY_NUM9", 57),
    ("KEY_COLON", 58),
    ("KEY_SEMICOLON", 59),
    ("KEY_EQUALS", 61),
    ("KEY_QUESTION", 63),
    ("KEY_A", 65),
    ("KEY_B", 66),
    ("KEY_C", 67),
    ("KEY_D", 68),
    ("KEY_E", 69),
    ("KEY_F", 70),
    ("KEY_G", 71),
    ("KEY_H", 72),
    ("KEY_I", 73),
    ("KEY_J", 74),
    ("KEY_K", 75),
    ("KEY_L", 76),
    ("KEY_M", 77),
    ("KEY_N", 78),
    ("KEY_O", 79),
    ("KEY_P", 80),
    ("KEY_Q", 81),
    ("KEY_R", 82),
    ("KEY_S", 83),
    ("KEY_T", 84),
    ("KEY_U", 85),
    ("KEY_V", 86),
    ("KEY_W", 87),
    ("KEY_X", 88),
    ("KEY_Y", 89),
    ("KEY_Z", 90),
    ("KEY_LEFT", 16001),
    ("KEY_RIGHT", 16002),
    ("KEY_UP", 16003),
    ("KEY_DOWN", 16004),
    ("KEY_SHIFT", 16005),
    ("AUDIO_FORMAT_I16", 0),
    ("NET_EOF", 0),
    ("NET_ERROR", -1),
    ("NET_TIMEOUT", -2),
    ("OPEN_READ", 1),
    ("OPEN_WRITE", 2),
    ("OPEN_CREATE", 4),
    ("OPEN_TRUNC", 8),
];
