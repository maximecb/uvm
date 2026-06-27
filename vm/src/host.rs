extern crate sdl2;
use std::collections::HashMap;
use std::cell::RefCell;
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
pub enum HostFn
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

impl HostFn
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

thread_local! {
    /// SDL context (used for UI and audio)
    /// This is thread-local because it doesn't implement the Send trait,
    /// and so can't be referenced from another thread
    static SDL: RefCell<Option<sdl2::Sdl>> = RefCell::new(None);
}

/// Get a handle to the SDL context, lazily initializing it
/// Sdl is a cheap reference-counted handle, so we return it by value
pub fn get_sdl_context() -> sdl2::Sdl
{
    SDL.with_borrow_mut(|sdl| {
        if sdl.is_none() {
            *sdl = Some(sdl2::init().unwrap());
        }

        sdl.as_ref().unwrap().clone()
    })
}

/// Get the syscall with a given index
pub fn get_syscall(const_idx: u16) -> HostFn
{
    match const_idx {
        // Core VM syscalls
        VM_HEAP_SIZE => HostFn::Fn0_1(vm_heap_size),
        VM_GROW_HEAP => HostFn::Fn1_1(vm_grow_heap),
        MEMSET => HostFn::Fn3_0(memset),
        MEMSET32 => HostFn::Fn3_0(memset32),
        MEMCPY => HostFn::Fn3_0(memcpy),
        MEMCMP => HostFn::Fn3_1(memcmp),
        EXIT => HostFn::Fn1_0(exit),

        THREAD_SPAWN => HostFn::Fn2_1(thread_spawn),
        THREAD_JOIN => HostFn::Fn1_1(thread_join),
        THREAD_ID => HostFn::Fn0_1(thread_id),
        THREAD_SLEEP => HostFn::Fn1_0(thread_sleep),

        // Console I/O
        PRINT_I64 => HostFn::Fn1_0(print_i64),
        PRINT_F32 => HostFn::Fn1_0(print_f32),
        PRINT_STR => HostFn::Fn1_0(print_str),
        PRINT_ENDL => HostFn::Fn0_0(print_endl),
        PUTCHAR => HostFn::Fn1_1(putchar),
        GETCHAR => HostFn::Fn0_1(getchar),

        TIME_CURRENT_MS => HostFn::Fn0_1(time_current_ms),

        WINDOW_CREATE => HostFn::Fn4_1(window_create),
        WINDOW_DRAW_FRAME => HostFn::Fn2_0(window_draw_frame),
        WINDOW_POLL_EVENT => HostFn::Fn1_1(window_poll_event),
        WINDOW_WAIT_EVENT => HostFn::Fn1_0(window_wait_event),

        AUDIO_OPEN_OUTPUT => HostFn::Fn4_1(audio_open_output),
        AUDIO_OPEN_INPUT => HostFn::Fn4_1(audio_open_input),
        AUDIO_READ_SAMPLES => HostFn::Fn2_0(audio_read_samples),

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
fn thread_spawn(thread: &mut Thread, fun: Value, arg: Value) -> Value
{
    let callee_pc = fun.as_u64();
    let tid = VM::spawn_thread(&thread.vm, callee_pc, vec![arg]);
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

// End program execution
fn exit(thread: &mut Thread, val: Value)
{
    unsafe { libc::exit(val.as_i32() & 0xFF) };
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

/// Do some basic safety checking (sandboxing) to minimize
/// security risks for file accesses
fn is_safe_path(file_path: &str) -> bool
{
    use std::path::PathBuf;
    use std::fs::canonicalize;

    let file_path = file_path.trim();
    let mut file_path = PathBuf::from(file_path);

    // Reject extensions associated with executable, script or
    // loadable library files. The comparison is case-insensitive
    // because some filesystems (e.g. macOS, Windows) treat "EXE"
    // and "exe" as referring to the same file, so a case-sensitive
    // check would be trivially bypassable.
    if let Some(ext) = file_path.extension() {
        match ext.to_string_lossy().to_lowercase().as_str() {
            // Windows executables and scripts
            "exe" | "com" | "scr" | "msi" | "cpl" | "dll" |
            "bat" | "cmd" | "ps1" | "psm1" | "vbs" | "vbe" |
            "js" | "jse" | "wsf" | "wsh" | "hta" | "jar" |
            // Unix/macOS executables, libraries and shell scripts
            "sh" | "bash" | "zsh" | "csh" | "ksh" | "fish" |
            "command" | "so" | "dylib" | "app" | "out" |
            // Interpreted language sources
            "py" | "pyc" | "pyo" | "rb" | "pl" | "php" | "lua"
                => return false,
            _ => {}
        }
    }

    // If this is a file that does not exist yet, pop the trailing
    // components from the path. This is necessary for the canonicalize
    // function to work
    while !file_path.exists() {
        file_path.pop();

        if file_path.as_os_str().is_empty() {
            file_path = PathBuf::from(".");
        }
    }

    // Get the absolute path for the file, resolving symlinks
    let file_path = canonicalize(&file_path).unwrap();
    //println!("Canonical path: {:?}", file_path);

    // Don't allow access to the current executable
    let current_exe = std::env::current_exe().unwrap();
    let current_exe = canonicalize(&current_exe).unwrap();
    if file_path == current_exe {
        println!("file path is current exe");
        return false;
    }

    // On Unix/Linux platforms, deny access to files marked as executable
    #[cfg(unix)]
    if file_path.exists() && !file_path.is_dir() {
        use std::os::unix::fs::PermissionsExt;
        let metadata = std::fs::metadata(&file_path).unwrap();
        let permissions = metadata.permissions();
        let mode = permissions.mode();
        if (mode & 0o111) != 0 {
            println!("mode is executable");
            return false;
        }
    }

    // Get the current working directory
    let cwd = std::env::current_dir().unwrap();
    let cwd = canonicalize(&cwd).unwrap();
    //println!("Canonical cwd: {:?}", cwd);

    // If the file path is inside the current working directory, allow access
    if file_path.starts_with(cwd) {
        return true;
    }

    /*
    // Parse the rest arguments
    let rest_args = crate::parse_args(std::env::args().collect()).rest;

    // For each rest argument supplied on the command-line
    for arg in rest_args {

        let arg_path = PathBuf::from(arg);

        // If this is not a valid path, ignore it
        if !arg_path.exists() {
            continue;
        }

        let arg_path = canonicalize(&arg_path).unwrap();

        // We can allow access to files in directories
        // explicitly specified on the command-line
        if arg_path.is_dir() {
            if file_path.starts_with(&arg_path) {
                return true;
            }
        }

        // We can allow access to files explicitly
        // specified on the command-line
        if arg_path.is_file() && file_path == arg_path {
            return true;
        }
    }
    */

    false
}

#[cfg(test)]
mod tests
{
    use crate::host::is_safe_path;

    #[test]
    fn safe_path()
    {
        assert!(!is_safe_path("/"));
        assert!(!is_safe_path("/root"));
        assert!(!is_safe_path("/usr/bin"));
        assert!(!is_safe_path("/home/user"));
        assert!(!is_safe_path(".."));
        assert!(!is_safe_path("run_me.sh"));
        assert!(!is_safe_path("run_me.exe"));

        // Other executable/script/library extensions are unsafe
        assert!(!is_safe_path("lib.dylib"));
        assert!(!is_safe_path("script.py"));
        assert!(!is_safe_path("app.jar"));

        // The blocklist must not be bypassable by changing the case
        assert!(!is_safe_path("run_me.SH"));
        assert!(!is_safe_path("MALWARE.Exe"));

        // Home directory access is not safe
        if let Some(home_path) = std::env::home_dir() {
            let home_path = home_path.to_str().unwrap();
            assert!(!is_safe_path(home_path));
        }

        // Safe paths inside CWD
        assert!(is_safe_path("."));
        assert!(is_safe_path("foo.txt"));
        assert!(is_safe_path("data.csv"));
        assert!(is_safe_path("docs/language.md"));
    }
}
