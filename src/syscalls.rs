use std::collections::HashMap;
use crate::vm::{VM};
use crate::window::*;
use crate::audio::*;

/// System call function signature
pub type SyscallFn = fn(&mut VM);

/// Map of names to syscall functions
static mut SYSCALLS: Option<HashMap::<String, SyscallFn>> = None;

fn hello_world(vm: &mut VM)
{
    println!("Hello World!");
}

fn reg_syscall(syscalls: &mut HashMap::<String, SyscallFn>, name: &str, fun: SyscallFn)
{
    syscalls.insert(name.to_string(), fun);
}

pub fn init_syscalls()
{
    // TODO: for now just set them here by hand

    let mut syscalls = HashMap::<String, SyscallFn>::new();

    reg_syscall(&mut syscalls, "hello_world", hello_world);

    reg_syscall(&mut syscalls, "create_window", create_window);





    unsafe {
        SYSCALLS = Some(syscalls)
    }
}

pub fn get_syscall(name: &str) -> SyscallFn
{
    unsafe {
        *SYSCALLS.as_ref().unwrap().get(name).unwrap()
    }
}
