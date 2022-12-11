use std::collections::HashMap;
use crate::vm::{Value, VM};
use crate::window::*;
use crate::audio::*;

/// System call function signature
pub type SysCallFn = fn(&mut VM);
pub type SysCallFn0_0 = fn(&mut VM);
pub type SysCallFn1_0 = fn(&mut VM, a0: Value);

/// System call descriptor
/// Note: the in/out arg count should be fixed so
///       that we can JIT syscalls effectively
struct SysCall
{
    host_fn: SysCallFn,

    // Number of input parameters
    num_ins: usize,

    // Number of outputs produced (currently has to be 0 or 1)
    num_outs: usize,
}

/// Map of names to syscall functions
static mut SYSCALLS: Option<HashMap::<String, SysCallFn>> = None;

fn print_i64(vm: &mut VM)
{
    let v = vm.pop().as_i64();
    println!("{}", v);
}

fn reg_syscall(
    syscalls: &mut HashMap::<String, SysCallFn>,
    name: &str,
    fun: SysCallFn,
    num_ins: usize,
    num_outs: usize
)
{
    assert!(num_ins <= 1);
    assert!(num_outs <= 1);

    syscalls.insert(name.to_string(), fun);
}

pub fn init_syscalls()
{
    // TODO: for now just set them here by hand

    let mut syscalls = HashMap::<String, SysCallFn>::new();

    reg_syscall(&mut syscalls, "print_i64", print_i64, 1, 0);

    reg_syscall(&mut syscalls, "window_create", window_create, 0, 0);
    reg_syscall(&mut syscalls, "window_show", window_show, 0, 0);
    reg_syscall(&mut syscalls, "window_copy_pixels", window_copy_pixels, 1, 0);

    unsafe {
        SYSCALLS = Some(syscalls)
    }
}

pub fn get_syscall(name: &str) -> SysCallFn
{
    unsafe {
        *SYSCALLS.as_ref().unwrap().get(name).unwrap()
    }
}
