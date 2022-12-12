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
    Fn0_1(fn(&mut VM) -> Value),
    Fn1_0(fn(&mut VM, a0: Value)),
    Fn2_0(fn(&mut VM, a0: Value, a1: Value)),
}

/// Map of names to syscall functions
static mut SYSCALLS: Option<HashMap::<String, SysCallFn>> = None;

fn print_i64(vm: &mut VM, v: Value)
{
    let v = v.as_i64();
    println!("{}", v);
}

/// Print a null-terminated UTF-8 string to stdout
fn print_str(vm: &mut VM, str_ptr: Value)
{
    use std::ffi::CStr;
    let char_ptr = vm.get_heap_ptr(str_ptr.as_usize());
    let c_str = unsafe { CStr::from_ptr(char_ptr as *const i8) };
    let rust_str = c_str.to_str().unwrap();

    println!("{}", rust_str);
}

fn read_i64(vm: &mut VM) -> Value
{
    let mut line_buf = String::new();
    std::io::stdin()
        .read_line(&mut line_buf)
        .expect("failed to read input line");
    let val: i64 = line_buf.trim().parse().expect("expected i64 input");

    return Value::from(val);
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
    reg_syscall(&mut syscalls, "print_str", SysCallFn::Fn1_0(print_str));
    reg_syscall(&mut syscalls, "read_i64", SysCallFn::Fn0_1(read_i64));

    reg_syscall(&mut syscalls, "window_create", SysCallFn::Fn2_0(window_create));
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
