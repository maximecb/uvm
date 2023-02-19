pub mod window;
pub mod audio;
pub mod time;
pub mod constants;

extern crate sdl2;
use std::collections::HashMap;
use std::io::Write;
use std::io::{stdout, stdin};
use crate::vm::{Value, VM};
use window::*;
use audio::*;
use time::*;
use constants::*;

/// System call function signature
/// Note: the in/out arg count should be fixed so
///       that we can JIT syscalls efficiently
#[derive(Copy, Clone)]
pub enum SysCallFn
{
    Fn0_0(fn(&mut VM)),
    Fn0_1(fn(&mut VM) -> Value),
    Fn1_0(fn(&mut VM, a0: Value)),
    Fn2_0(fn(&mut VM, a0: Value, a1: Value)),
    Fn3_0(fn(&mut VM, a0: Value, a1: Value, a2: Value)),
    Fn4_0(fn(&mut VM, a0: Value, a1: Value, a2: Value, a3: Value)),
    Fn4_1(fn(&mut VM, a0: Value, a1: Value, a2: Value, a3: Value) -> Value),
}

impl SysCallFn
{
    fn argc(&self) -> usize
    {
        match self {
            Self::Fn0_0(_) => 0,
            Self::Fn0_1(_) => 0,
            Self::Fn1_0(_) => 1,
            Self::Fn2_0(_) => 2,
            Self::Fn3_0(_) => 3,
            Self::Fn4_0(_) => 4,
            Self::Fn4_1(_) => 4,
        }
    }

    fn has_ret(&self) -> bool
    {
        match self {
            Self::Fn0_0(_) => false,
            Self::Fn0_1(_) => true,
            Self::Fn1_0(_) => false,
            Self::Fn2_0(_) => false,
            Self::Fn3_0(_) => false,
            Self::Fn4_0(_) => false,
            Self::Fn4_1(_) => true,
        }
    }
}

pub struct SysState
{
    /// Map of indices to syscall functions
    syscalls: [Option<SysCallFn>; SYSCALL_TBL_LEN],

    /// SDL context (used for UI and audio)
    sdl: Option<sdl2::Sdl>,

    /// Window module state
    pub window_state: Option<WindowState>,

    // Time module state
    pub time_state: TimeState,
}

impl SysState
{
    pub fn new() -> Self
    {
        let mut sys_state = Self {
            syscalls: [None; SYSCALL_TBL_LEN],
            sdl: None,
            window_state: None,
            time_state: TimeState::new(),
        };

        sys_state.init_syscalls();

        sys_state
    }

    pub fn get_sdl_context(&mut self) -> &mut sdl2::Sdl
    {
        // Lazily initialize the SDL context
        if self.sdl.is_none() {
            self.sdl = Some(sdl2::init().unwrap());
        }

        self.sdl.as_mut().unwrap()
    }

    pub fn reg_syscall(&mut self, const_idx: u16, fun: SysCallFn)
    {
        let desc = SYSCALL_DESCS[const_idx as usize].as_ref().unwrap();

        assert!(
            fun.argc() == desc.argc,
            "{} should accept {} args but implementation has {} params",
            desc.name,
            desc.argc,
            fun.argc()
        );

        assert!(fun.has_ret() == desc.has_ret);

        self.syscalls[const_idx as usize] = Some(fun);
    }

    /// Get the syscall with a given index
    pub fn get_syscall(&self, const_idx: u16) -> SysCallFn
    {
        if let Some(syscall_fn) = self.syscalls[const_idx as usize] {
            return syscall_fn;
        }
        else
        {
            panic!("unknown syscall \"{}\"", const_idx);
        }
    }

    fn init_syscalls(&mut self)
    {
        let mut syscalls = HashMap::<String, SysCallFn>::new();

        self.reg_syscall(MEMSET, SysCallFn::Fn3_0(memset));
        self.reg_syscall(MEMCPY, SysCallFn::Fn3_0(memcpy));
        self.reg_syscall(VM_HEAP_SIZE, SysCallFn::Fn0_1(vm_heap_size));
        self.reg_syscall(VM_RESIZE_HEAP, SysCallFn::Fn1_0(vm_resize_heap));

        self.reg_syscall(PRINT_I64, SysCallFn::Fn1_0(print_i64));
        self.reg_syscall(PRINT_STR, SysCallFn::Fn1_0(print_str));
        self.reg_syscall(PRINT_ENDL, SysCallFn::Fn0_0(print_endl));
        self.reg_syscall(READ_I64, SysCallFn::Fn0_1(read_i64));

        self.reg_syscall(TIME_CURRENT_MS, SysCallFn::Fn0_1(time_current_ms));
        self.reg_syscall(TIME_DELAY_CB, SysCallFn::Fn2_0(time_delay_cb));

        self.reg_syscall(WINDOW_CREATE, SysCallFn::Fn4_1(window_create));
        self.reg_syscall(WINDOW_DRAW_FRAME, SysCallFn::Fn2_0(window_draw_frame));
        self.reg_syscall(WINDOW_ON_MOUSEMOVE, SysCallFn::Fn2_0(window_on_mousemove));
        self.reg_syscall(WINDOW_ON_MOUSEDOWN, SysCallFn::Fn2_0(window_on_mousedown));
        self.reg_syscall(WINDOW_ON_MOUSEUP, SysCallFn::Fn2_0(window_on_mouseup));
        self.reg_syscall(WINDOW_ON_KEYDOWN, SysCallFn::Fn2_0(window_on_keydown));
        self.reg_syscall(WINDOW_ON_KEYUP, SysCallFn::Fn2_0(window_on_keyup));
    }
}

fn vm_heap_size(vm: &mut VM) -> Value
{
    Value::from(vm.heap_size())
}

fn vm_resize_heap(vm: &mut VM, num_bytes: Value)
{
    let num_bytes = num_bytes.as_usize();
    vm.resize_heap(num_bytes);
}

fn memset(vm: &mut VM, dst_ptr: Value, val: Value, num_bytes: Value)
{
    let dst_ptr = dst_ptr.as_usize();
    let val = val.as_u8();
    let num_bytes = num_bytes.as_usize();

    let mem_slice: &mut [u8] = vm.get_heap_slice(dst_ptr, num_bytes);
    mem_slice.fill(val);
}

fn memcpy(vm: &mut VM, dst_ptr: Value, src_ptr: Value, num_bytes: Value)
{
    let dst_ptr = dst_ptr.as_usize();
    let src_ptr = src_ptr.as_usize();
    let num_bytes = num_bytes.as_usize();

    // TODO: panic if slices are overlapping

    let dst_ptr: *mut u8 = vm.get_heap_ptr(dst_ptr);
    let src_ptr: *mut u8 = vm.get_heap_ptr(src_ptr);

    unsafe {
        std::ptr::copy_nonoverlapping(src_ptr, dst_ptr, num_bytes);
    }
}

fn print_i64(vm: &mut VM, v: Value)
{
    let v = v.as_i64();
    print!("{}", v);
    stdout().flush().unwrap();
}

/// Print a null-terminated UTF-8 string to stdout
fn print_str(vm: &mut VM, str_ptr: Value)
{
    let rust_str = vm.get_heap_str(str_ptr.as_usize());
    print!("{}", rust_str);
    stdout().flush().unwrap();
}

/// Print a newline characted to stdout
fn print_endl(vm: &mut VM)
{
    println!();
    stdout().flush().unwrap();
}

fn read_i64(vm: &mut VM) -> Value
{
    let mut line_buf = String::new();
    stdin()
        .read_line(&mut line_buf)
        .expect("failed to read input line");
    let val: i64 = line_buf.trim().parse().expect("expected i64 input");

    return Value::from(val);
}
