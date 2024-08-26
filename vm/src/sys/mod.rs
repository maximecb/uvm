pub mod window;
pub mod audio;
pub mod net;
pub mod time;
pub mod constants;

extern crate sdl2;
use std::collections::HashMap;
use std::io::Write;
use std::io::Read;
use std::io::{stdout, stdin};
use std::sync::{Arc, Weak, Mutex};
use crate::vm::{Value, VM};
use window::*;
use audio::*;
use net::*;
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
    Fn1_1(fn(&mut VM, a0: Value) -> Value),

    Fn2_0(fn(&mut VM, a0: Value, a1: Value)),
    Fn2_1(fn(&mut VM, a0: Value, a1: Value) -> Value),

    Fn3_0(fn(&mut VM, a0: Value, a1: Value, a2: Value)),
    Fn3_1(fn(&mut VM, a0: Value, a1: Value, a2: Value) -> Value),

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
            Self::Fn1_1(_) => 1,
            Self::Fn2_0(_) => 2,
            Self::Fn2_1(_) => 2,
            Self::Fn3_0(_) => 3,
            Self::Fn3_1(_) => 3,
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
            Self::Fn1_1(_) => true,
            Self::Fn2_0(_) => false,
            Self::Fn2_1(_) => true,
            Self::Fn3_0(_) => false,
            Self::Fn3_1(_) => true,
            Self::Fn4_0(_) => false,
            Self::Fn4_1(_) => true,
        }
    }
}

/// SDL context (used for UI and audio)
/// This is a global variable because it doesn't implement
/// the Send trait, and so can't be referenced from another thread
static mut SDL: Option<sdl2::Sdl> = None;

pub fn get_sdl_context() -> &'static mut sdl2::Sdl
{
    unsafe
    {
        // Lazily initialize the SDL context
        if SDL.is_none() {
            SDL = Some(sdl2::init().unwrap());
        }

        SDL.as_mut().unwrap()
    }
}

pub struct SysState
{
    /// Map of indices to syscall functions
    syscalls: [Option<SysCallFn>; SYSCALL_TBL_LEN],

    /// Weak reference to a mutex for the VM
    mutex: Weak<Mutex<VM>>,



}

impl SysState
{
    pub fn new() -> Self
    {
        let mut sys_state = Self {
            syscalls: [None; SYSCALL_TBL_LEN],
            mutex: Weak::new(),
            //net_state: NetState::default(),
        };

        sys_state.init_syscalls();

        sys_state
    }

    pub fn get_mutex(vm: VM) -> Arc<Mutex<VM>>
    {
        // Move the VM into a mutex
        let vm_arc = Arc::new(Mutex::new(vm));

        // Store a weak reference to the mutex into the sys state
        vm_arc.lock().unwrap().sys_state.mutex = Arc::downgrade(&vm_arc);

        vm_arc
    }

    /// Register a syscall implementation
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

        // Core VM syscalls
        self.reg_syscall(VM_HEAP_SIZE, SysCallFn::Fn0_1(vm_heap_size));
        self.reg_syscall(VM_RESIZE_HEAP, SysCallFn::Fn1_1(vm_resize_heap));
        self.reg_syscall(MEMSET, SysCallFn::Fn3_0(memset));
        self.reg_syscall(MEMSET32, SysCallFn::Fn3_0(memset32));
        self.reg_syscall(MEMCPY, SysCallFn::Fn3_0(memcpy));
        self.reg_syscall(MEMCMP, SysCallFn::Fn3_1(memcmp));

        self.reg_syscall(PRINT_I64, SysCallFn::Fn1_0(print_i64));
        self.reg_syscall(PRINT_F32, SysCallFn::Fn1_0(print_f32));
        self.reg_syscall(PRINT_STR, SysCallFn::Fn1_0(print_str));
        self.reg_syscall(PRINT_ENDL, SysCallFn::Fn0_0(print_endl));
        self.reg_syscall(PUTCHAR, SysCallFn::Fn1_1(putchar));
        self.reg_syscall(GETCHAR, SysCallFn::Fn0_1(getchar));

        self.reg_syscall(TIME_CURRENT_MS, SysCallFn::Fn0_1(time_current_ms));
        //self.reg_syscall(TIME_DELAY_CB, SysCallFn::Fn2_0(time_delay_cb));

        self.reg_syscall(WINDOW_CREATE, SysCallFn::Fn4_1(window_create));
        self.reg_syscall(WINDOW_DRAW_FRAME, SysCallFn::Fn2_0(window_draw_frame));

        self.reg_syscall(AUDIO_OPEN_OUTPUT, SysCallFn::Fn4_1(audio_open_output));

        //self.reg_syscall(NET_LISTEN, SysCallFn::Fn2_1(net_listen));
        //self.reg_syscall(NET_ACCEPT, SysCallFn::Fn4_1(net_accept));
        //self.reg_syscall(NET_READ, SysCallFn::Fn3_1(net_read));
        //self.reg_syscall(NET_WRITE, SysCallFn::Fn3_1(net_write));
        //self.reg_syscall(NET_CLOSE, SysCallFn::Fn1_0(net_close));
    }
}

fn vm_heap_size(vm: &mut VM) -> Value
{
    Value::from(vm.heap_size())
}

fn vm_resize_heap(vm: &mut VM, num_bytes: Value) -> Value
{
    let num_bytes = num_bytes.as_usize();
    let new_size = vm.resize_heap(num_bytes);
    Value::from(new_size)
}

fn memset(vm: &mut VM, dst_ptr: Value, val: Value, num_bytes: Value)
{
    let dst_ptr = dst_ptr.as_usize();
    let val = val.as_u8();
    let num_bytes = num_bytes.as_usize();

    let mem_slice: &mut [u8] = vm.get_heap_slice(dst_ptr, num_bytes);
    mem_slice.fill(val);
}

fn memset32(vm: &mut VM, dst_ptr: Value, word: Value, num_words: Value)
{
    let dst_ptr = dst_ptr.as_usize();
    let word = word.as_u32();
    let num_words = num_words.as_usize();

    let mem_slice: &mut [u32] = vm.get_heap_slice(dst_ptr, num_words);
    mem_slice.fill(word);
}

fn memcpy(vm: &mut VM, dst_ptr: Value, src_ptr: Value, num_bytes: Value)
{
    let dst_ptr = dst_ptr.as_usize();
    let src_ptr = src_ptr.as_usize();
    let num_bytes = num_bytes.as_usize();

    // TODO: panic if slices are overlapping

    unsafe {
        let dst_ptr: *mut u8 = vm.get_heap_ptr(dst_ptr, num_bytes);
        let src_ptr: *mut u8 = vm.get_heap_ptr(src_ptr, num_bytes);

        std::ptr::copy_nonoverlapping(src_ptr, dst_ptr, num_bytes);
    }
}

fn memcmp(vm: &mut VM, ptr_a: Value, ptr_b: Value, num_bytes: Value) -> Value
{
    let num_bytes = num_bytes.as_usize();

    unsafe {
        let ptr_a: *const libc::c_void = vm.get_heap_ptr(ptr_a.as_usize(), num_bytes);
        let ptr_b: *const libc::c_void  = vm.get_heap_ptr(ptr_b.as_usize(), num_bytes);

        let result = libc::memcmp(ptr_a, ptr_b, num_bytes);

        Value::from(result as u64)
    }
}

fn print_i64(vm: &mut VM, v: Value)
{
    let v = v.as_i64();
    print!("{}", v);
}

fn print_f32(vm: &mut VM, v: Value)
{
    let v = v.as_f32();
    print!("{}", v);
}

/// Print a null-terminated UTF-8 string to stdout
fn print_str(vm: &mut VM, str_ptr: Value)
{
    let rust_str = vm.get_heap_str(str_ptr.as_usize());
    print!("{}", rust_str);
}

/// Print a newline characted to stdout
fn print_endl(vm: &mut VM)
{
    println!();
}

/// Write one byte of input to stdout.
/// Analogous to C's getchar
fn putchar(vm: &mut VM, byte: Value) -> Value
{
    let byte = byte.as_u8();
    let bytes = byte.to_le_bytes();

    match stdout().write_all(&bytes) {
        Ok(_) => Value::from(byte),
        Err(_) => Value::from(-1 as i64),
    }
}

/// Read one byte of input from stdin.
/// Analogous to C's getchar
fn getchar(vm: &mut VM) -> Value
{
    let ch = stdin().bytes().next();

    match ch {
        Some(Ok(ch)) => Value::from(ch as i64),
        None | Some(Err(_)) => Value::from(-1 as i64),
    }
}
