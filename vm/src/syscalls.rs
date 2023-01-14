extern crate sdl2;
use std::collections::HashMap;
use std::io::Write;
use std::io::{stdout, stdin};
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
    Fn3_0(fn(&mut VM, a0: Value, a1: Value, a2: Value)),
}

pub struct SysState
{
    /// Map of names to syscall functions
    syscalls: HashMap::<String, SysCallFn>,

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
            syscalls: HashMap::default(),
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

    pub fn reg_syscall(&mut self, name: &str, fun: SysCallFn)
    {
        self.syscalls.insert(name.to_string(), fun);
    }

    /// Get the syscall with a given name string
    pub fn get_syscall(&self, name: &str) -> SysCallFn
    {
        if let Some(syscall_fn) = self.syscalls.get(name) {
            return *syscall_fn;
        }
        else
        {
            panic!("unknown syscall \"{}\"", name);
        }
    }

    fn init_syscalls(&mut self)
    {
        let mut syscalls = HashMap::<String, SysCallFn>::new();

        //TODO:
        //memcpy(dst, src, num_bytes)
        //memset(ptr, val, num_bytes)
        //vm_resize_heap(new_size)

        self.reg_syscall("memset", SysCallFn::Fn3_0(memset));

        self.reg_syscall("print_i64", SysCallFn::Fn1_0(print_i64));
        self.reg_syscall("print_str", SysCallFn::Fn1_0(print_str));
        self.reg_syscall("print_endl", SysCallFn::Fn0_0(print_endl));
        self.reg_syscall("read_i64", SysCallFn::Fn0_1(read_i64));

        self.reg_syscall("time_current_ms", SysCallFn::Fn0_1(time_current_ms));
        self.reg_syscall("time_delay_cb", SysCallFn::Fn2_0(time_delay_cb));

        self.reg_syscall("window_create", SysCallFn::Fn3_0(window_create));
        self.reg_syscall("window_show", SysCallFn::Fn0_0(window_show));
        self.reg_syscall("window_copy_pixels", SysCallFn::Fn1_0(window_copy_pixels));
        self.reg_syscall("window_on_mousemove", SysCallFn::Fn2_0(window_on_mousemove));
        self.reg_syscall("window_on_mousedown", SysCallFn::Fn2_0(window_on_mousedown));
        self.reg_syscall("window_on_mouseup", SysCallFn::Fn2_0(window_on_mouseup));
    }
}

fn memset(vm: &mut VM, dst_ptr: Value, val: Value, num_bytes: Value)
{
    let dst_ptr = dst_ptr.as_usize();
    let val = val.as_u8();
    let num_bytes = num_bytes.as_usize();

    let mem_slice: &mut [u8] = vm.get_heap_slice(dst_ptr, num_bytes);
    mem_slice.fill(val);
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
