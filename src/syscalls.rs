use std::collections::HashMap;
use crate::vm::{Value, VM};
use crate::window::*;
use crate::audio::*;

/// System call function signature
/// Note: the in/out arg count should be fixed so
///       that we can JIT syscalls effectively
#[derive(Copy, Clone)]
pub enum SysCallFn
{
    Fn0_0(fn(&mut VM)),
    Fn1_0(fn(&mut VM, a0: Value)),
}

/// Map of names to syscall functions
static mut SYSCALLS: Option<HashMap::<String, SysCallFn>> = None;

fn print_i64(vm: &mut VM, v: Value)
{
    let v = v.as_i64();
    println!("{}", v);
}

fn reg_syscall(syscalls: &mut HashMap::<String, SysCallFn>, name: &str, fun: SysCallFn)
{
    syscalls.insert(name.to_string(), fun);
}

pub fn init_syscalls()
{
    let mut syscalls = HashMap::<String, SysCallFn>::new();

    //TODO:
    //vm_resize_heap(new_size)

    reg_syscall(&mut syscalls, "print_i64", SysCallFn::Fn1_0(print_i64));

    reg_syscall(&mut syscalls, "window_create", SysCallFn::Fn0_0(window_create));
    reg_syscall(&mut syscalls, "window_show", SysCallFn::Fn0_0(window_show));
    reg_syscall(&mut syscalls, "window_copy_pixels", SysCallFn::Fn1_0(window_copy_pixels));

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
