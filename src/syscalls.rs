extern crate sdl2;
use std::collections::HashMap;
use crate::vm::{Value, VM};
use crate::window::*;
use crate::audio::*;
use crate::time::*;

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

pub struct SysState
{
    /// Map of names to syscall functions
    syscalls: HashMap::<String, SysCallFn>,

    /// SDL context (used for UI and audio)
    sdl: Option<sdl2::Sdl>,

    /// Window syscall state
    pub window_state: Option<WindowState>,
}

impl SysState
{
    pub fn new() -> Self
    {
        let mut sys_state = Self {
            syscalls: HashMap::default(),
            sdl: None,
            window_state: None,
        };

        sys_state.init_syscalls();

        sys_state
    }

    pub fn get_sdl_context(&mut self) -> &mut sdl2::Sdl
    {
        if self.sdl.is_none() {
            self.sdl = Some(sdl2::init().unwrap());
        }

        self.sdl.as_mut().unwrap()
    }

    pub fn reg_syscall(&mut self, name: &str, fun: SysCallFn)
    {
        self.syscalls.insert(name.to_string(), fun);
    }

    pub fn get_syscall(&self, name: &str) -> SysCallFn
    {
        *self.syscalls.get(name).unwrap()
    }

    fn init_syscalls(&mut self)
    {
        let mut syscalls = HashMap::<String, SysCallFn>::new();

        //TODO:
        //vm_resize_heap(new_size)

        self.reg_syscall("print_i64", SysCallFn::Fn1_0(print_i64));
        self.reg_syscall("print_str", SysCallFn::Fn1_0(print_str));
        self.reg_syscall("read_i64", SysCallFn::Fn0_1(read_i64));

        self.reg_syscall("time_current_ms", SysCallFn::Fn0_1(time_current_ms));

        self.reg_syscall("window_create", SysCallFn::Fn2_0(window_create));
        self.reg_syscall("window_show", SysCallFn::Fn0_0(window_show));
        self.reg_syscall("window_copy_pixels", SysCallFn::Fn1_0(window_copy_pixels));
    }
}

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
