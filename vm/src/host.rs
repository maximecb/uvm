extern crate sdl2;
use std::collections::HashMap;
use std::io::Write;
use std::io::Read;
use std::io::{stdout, stdin};
use std::sync::{Arc, Weak, Mutex};
use crate::vm::{Value, VM, Thread};
use crate::window::*;
use crate::audio::*;
use crate::net::*;
use crate::time::*;
use crate::constants::*;

/// System call function signature
/// Note: the in/out arg count should be fixed so
///       that we can JIT syscalls efficiently
#[derive(Copy, Clone)]
pub enum SysCallFn
{
    Fn0_0(fn(&mut Thread)),
    Fn0_1(fn(&mut Thread) -> Value),

    Fn1_0(fn(&mut Thread, a0: Value)),
    Fn1_1(fn(&mut Thread, a0: Value) -> Value),

    Fn2_0(fn(&mut Thread, a0: Value, a1: Value)),
    Fn2_1(fn(&mut Thread, a0: Value, a1: Value) -> Value),

    Fn3_0(fn(&mut Thread, a0: Value, a1: Value, a2: Value)),
    Fn3_1(fn(&mut Thread, a0: Value, a1: Value, a2: Value) -> Value),

    Fn4_0(fn(&mut Thread, a0: Value, a1: Value, a2: Value, a3: Value)),
    Fn4_1(fn(&mut Thread, a0: Value, a1: Value, a2: Value, a3: Value) -> Value),
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

/// Get the syscall with a given index
pub fn get_syscall(const_idx: u16) -> SysCallFn
{
    match const_idx {
        // Core VM syscalls
        VM_HEAP_SIZE => SysCallFn::Fn0_1(vm_heap_size),
        VM_GROW_HEAP => SysCallFn::Fn1_1(vm_grow_heap),
        MEMSET => SysCallFn::Fn3_0(memset),
        MEMSET32 => SysCallFn::Fn3_0(memset32),
        MEMCPY => SysCallFn::Fn3_0(memcpy),
        MEMCMP => SysCallFn::Fn3_1(memcmp),

        THREAD_SPAWN => SysCallFn::Fn1_1(thread_spawn),
        THREAD_JOIN => SysCallFn::Fn1_1(thread_join),
        THREAD_ID => SysCallFn::Fn0_1(thread_id),
        THREAD_SLEEP => SysCallFn::Fn1_0(thread_sleep),

        // Console I/O
        PRINT_I64 => SysCallFn::Fn1_0(print_i64),
        PRINT_F32 => SysCallFn::Fn1_0(print_f32),
        PRINT_STR => SysCallFn::Fn1_0(print_str),
        PRINT_ENDL => SysCallFn::Fn0_0(print_endl),
        PUTCHAR => SysCallFn::Fn1_1(putchar),
        GETCHAR => SysCallFn::Fn0_1(getchar),

        TIME_CURRENT_MS => SysCallFn::Fn0_1(time_current_ms),

        WINDOW_CREATE => SysCallFn::Fn4_1(window_create),
        WINDOW_DRAW_FRAME => SysCallFn::Fn2_0(window_draw_frame),
        WINDOW_POLL_EVENT => SysCallFn::Fn1_1(window_poll_event),
        WINDOW_WAIT_EVENT => SysCallFn::Fn1_0(window_wait_event),

        AUDIO_OPEN_OUTPUT => SysCallFn::Fn4_1(audio_open_output),

        _ => panic!("unknown syscall \"{}\"", const_idx),
    }
}

fn vm_heap_size(thread: &mut Thread) -> Value
{
    Value::from(thread.heap_size())
}

fn vm_grow_heap(thread: &mut Thread, num_bytes: Value) -> Value
{
    let mut vm = thread.vm.lock().unwrap();
    let num_bytes = num_bytes.as_usize();
    let new_size = vm.grow_heap(num_bytes);
    Value::from(new_size)
}

fn thread_id(thread: &mut Thread) -> Value
{
    Value::from(thread.id)
}

// Make the current thread sleep
fn thread_sleep(thread: &mut Thread, msecs: Value)
{
    use std::thread;
    use std::time::Duration;
    let msecs = msecs.as_u64();
    thread::sleep(Duration::from_millis(msecs));
}

// Spawn a new thread
// Takes a function to call as argument
// Returns a thread id
fn thread_spawn(thread: &mut Thread, fun: Value) -> Value
{
    let callee_pc = fun.as_u64();
    let tid = VM::spawn_thread(&thread.vm, callee_pc, vec![]);
    Value::from(tid)
}

// Wait for a thread to terminatr, produce the return value
fn thread_join(thread: &mut Thread, tid: Value) -> Value
{
    let tid = tid.as_u64();
    VM::join_thread(&thread.vm, tid)
}

fn memset(thread: &mut Thread, dst_ptr: Value, val: Value, num_bytes: Value)
{
    let dst_ptr = dst_ptr.as_usize();
    let val = val.as_u8();
    let num_bytes = num_bytes.as_usize();

    let mem_slice: &mut [u8] = thread.get_heap_slice_mut(dst_ptr, num_bytes);
    mem_slice.fill(val);
}

fn memset32(thread: &mut Thread, dst_ptr: Value, word: Value, num_words: Value)
{
    let dst_ptr = dst_ptr.as_usize();
    let word = word.as_u32();
    let num_words = num_words.as_usize();

    let mem_slice: &mut [u32] = thread.get_heap_slice_mut(dst_ptr, num_words);
    mem_slice.fill(word);
}

fn memcpy(thread: &mut Thread, dst_ptr: Value, src_ptr: Value, num_bytes: Value)
{
    let dst_ptr = dst_ptr.as_usize();
    let src_ptr = src_ptr.as_usize();
    let num_bytes = num_bytes.as_usize();

    let overlap = (
        (dst_ptr >= src_ptr && dst_ptr < src_ptr + num_bytes) ||
        (src_ptr >= dst_ptr && src_ptr < dst_ptr + num_bytes)
    );

    if overlap {
        panic!("memcpy to/from overlapping regions");
    }

    unsafe {
        let dst_ptr: *mut u8 = thread.get_heap_ptr_mut(dst_ptr, num_bytes);
        let src_ptr: *mut u8 = thread.get_heap_ptr_mut(src_ptr, num_bytes);

        std::ptr::copy_nonoverlapping(src_ptr, dst_ptr, num_bytes);
    }
}

fn memcmp(thread: &mut Thread, ptr_a: Value, ptr_b: Value, num_bytes: Value) -> Value
{
    let num_bytes = num_bytes.as_usize();

    unsafe {
        let ptr_a: *const libc::c_void = thread.get_heap_ptr_mut(ptr_a.as_usize(), num_bytes);
        let ptr_b: *const libc::c_void  = thread.get_heap_ptr_mut(ptr_b.as_usize(), num_bytes);

        let result = libc::memcmp(ptr_a, ptr_b, num_bytes);
        Value::from(result as u64)
    }
}

fn print_i64(thread: &mut Thread, v: Value)
{
    let v = v.as_i64();
    print!("{}", v);
}

fn print_f32(thread: &mut Thread, v: Value)
{
    let v = v.as_f32();
    print!("{}", v);
}

/// Print a null-terminated UTF-8 string to stdout
fn print_str(thread: &mut Thread, str_ptr: Value)
{
    let rust_str = thread.get_heap_str(str_ptr.as_usize());
    print!("{}", rust_str);
}

/// Print a newline characted to stdout
fn print_endl(thread: &mut Thread)
{
    println!();
}

/// Write one byte of input to stdout.
/// Analogous to C's getchar
fn putchar(thread: &mut Thread, byte: Value) -> Value
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
fn getchar(thread: &mut Thread) -> Value
{
    let ch = stdin().bytes().next();

    match ch {
        Some(Ok(ch)) => Value::from(ch as i64),
        None | Some(Err(_)) => Value::from(-1 as i64),
    }
}
