use std::collections::HashMap;
use crate::vm::{VM};

/// System call function signature
type SyscallFn = fn(&VM);

/// Map of names to syscall functions
static mut SYSCALLS: Option<HashMap::<String, SyscallFn>> = None;

fn hello_world(vm: &VM)
{
    println!("Hello World!");
}

pub fn init_syscalls()
{
    // TODO: for now just set them here by hand

    let mut syscalls = HashMap::<String, SyscallFn>::new();

    syscalls.insert("hello_world".to_string(), hello_world);






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
