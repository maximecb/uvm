//
// This file was automatically generated based on api/syscalls.json
//

#![allow(unused)]

const NUM_SYSCALLS: usize = 5;

const MEMCPY: u16 = 3;
const MEMSET: u16 = 4;
const TIME_CURRENT_MS: u16 = 0;
const TIME_DELAY_CB: u16 = 2;
const WINDOW_CREATE: u16 = 1;

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
];
