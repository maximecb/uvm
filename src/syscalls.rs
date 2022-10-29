use std::collections::HashMap;
use crate::vm::{VM};
use crate::window::*;
use crate::audio::*;

/// System call function signature
pub type SysCallFn = fn(&mut VM);

/// System call descriptor
/// Note: the in/out arg count should be fixed so
///       that we can JIT syscalls effectively
struct SysCall
{
    host_fn: SysCallFn,

    // Number of input parameters
    num_ins: usize,

    // Number of outputs produced (currently has to be zero or one)
    num_outs: usize,
}

/// Map of names to syscall functions
static mut SYSCALLS: Option<HashMap::<String, SysCallFn>> = None;

fn hello_world(vm: &mut VM)
{
    println!("Hello World!");
}

fn reg_syscall(syscalls: &mut HashMap::<String, SysCallFn>, name: &str, fun: SysCallFn)
{
    syscalls.insert(name.to_string(), fun);
}

pub fn init_syscalls()
{
    // TODO: for now just set them here by hand

    let mut syscalls = HashMap::<String, SysCallFn>::new();

    reg_syscall(&mut syscalls, "hello_world", hello_world);

    reg_syscall(&mut syscalls, "window_create", window_create);
    reg_syscall(&mut syscalls, "window_copy_pixels", window_copy_pixels);




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
