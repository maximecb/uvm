use std::sync::{Arc, Mutex};
use std::mem::{transmute, size_of};
use std::collections::{HashSet, HashMap};
use std::thread;
use std::ffi::CStr;
use crate::sys::*;

/// Instruction opcodes
/// Note: commonly used upcodes should be in the [0, 127] range (one byte)
///       less frequently used opcodes can take multiple bytes if necessary.
#[allow(non_camel_case_types)]
#[derive(PartialEq, Copy, Clone, Debug)]
#[repr(u8)]
pub enum Op
{
    // Halt execution and produce an error
    // Panic is zero so that jumping to uninitialized memory causes panic
    panic = 0,

    // No-op (useful for code patching or patch points)
    nop,

    // Debugger breakpoint.
    // This instruction must be just one byte so it can be patched anywhere.
    breakpoint,

    // Push common constants (0, 1, 2)
    push_0,
    push_1,
    push_2,

    // Push zero n times (e.g. initialize locals)
    // push_0n <n:u8>
    push_0n,

    // push_i8 <imm:i8> (sign-extended)
    push_i8,

    // push_u32 <imm:u32>
    push_u32,

    // push_u64 <imm:u64>
    push_u64,

    // Stack manipulation
    pop,
    dup,
    swap,

    // Push the nth-value (indexed from the stack top) on top of the stack
    // getn 0 is equivalent to dup
    // getn <idx:u8>
    getn,

    // Pop the stack top and set the nth stack slot from the top to this value
    // setn 0 is equivalent to removing the value below the current stack top
    // setn <idx:u8>
    setn,

    // Get the argument count for the current stack frame
    get_argc,

    // Get the function argument at a given index
    // get_arg <idx:u8>
    get_arg,

    // Get a variadic argument with a dynamic index variable
    // get_arg (idx)
    get_var_arg,

    // Set the function argument at a given index
    // set_arg <idx:u8> (value)
    set_arg,

    // Get the local variable at a given stack slot index
    // The index is relative to the base of the stack frame
    // get_local <idx:u8>
    get_local,

    // Set the local variable at a given stack slot index
    // The index is relative to the base of the stack frame
    // set_local <idx:u8> (value)
    set_local,

    // 32-bit bitwise operations
    and_u32,
    or_u32,
    xor_u32,
    not_u32,
    lshift_u32,
    rshift_u32,
    rshift_i32,

    // 32-bit integer arithmetic
    add_u32,
    sub_u32,
    mul_u32,
    div_u32,
    mod_u32,
    div_i32,
    mod_i32,

    // 32-bit integer comparisons
    eq_u32,
    ne_u32,
    lt_u32,
    le_u32,
    gt_u32,
    ge_u32,
    lt_i32,
    le_i32,
    gt_i32,
    ge_i32,

    // 64-bit bitwise operations
    and_u64,
    or_u64,
    xor_u64,
    not_u64,
    lshift_u64,
    rshift_u64,
    rshift_i64,

    // 64-bit integer arithmetic
    add_u64,
    sub_u64,
    mul_u64,
    div_u64,
    mod_u64,
    div_i64,
    mod_i64,

    // TODO: arithmetic with overflow
    // These instructions probably shouldn't jump directly,
    // as this would add more branch instructions to the
    // instruction set.
    // We don't need to worry about compactness.
    // add_u64_ovf,
    // sub_u64_ovf,
    // mul_i64_ovf, // produces two 64-bit words of output

    // 64-bit integer comparisons
    eq_u64,
    ne_u64,
    lt_u64,
    le_u64,
    gt_u64,
    ge_u64,
    lt_i64,
    le_i64,
    gt_i64,
    ge_i64,

    // Integer sign extension
    sx_i8_i32,
    sx_i8_i64,
    sx_i16_i32,
    sx_i16_i64,
    sx_i32_i64,

    // Truncation instructions
    trunc_u8,
    trunc_u16,
    trunc_u32,

    // 32-bit floating-point arithmetic
    add_f32,
    sub_f32,
    mul_f32,
    div_f32,

    // Floating-point math functions
    sin_f32,
    cos_f32,
    tan_f32,
    asin_f32,
    acos_f32,
    atan_f32,
    pow_f32,
    sqrt_f32,

    // 32-bit floating-point comparison instructions
    eq_f32,
    ne_f32,
    lt_f32,
    le_f32,
    gt_f32,
    ge_f32,

    // Int/float conversion
    i32_to_f32,
    i64_to_f32,
    f32_to_i32,

    // Load a value at a given adress
    // store (addr)
    load_u8,
    load_u16,
    load_u32,
    load_u64,

    // Store a value at a given adress
    // store (addr) (value)
    store_u8,
    store_u16,
    store_u32,
    store_u64,

    /*
    // TODO:
    // Load from heap at fixed address
    // This is used for reading global variables
    // The address is multiplied by the data size (x 4 or x8)
    // If we save 24 bits for the offset, then that gives us quite a lot
    load_global_u64 <addr:u24>
    */

    // NOTE: may want to wait for this because it's not RISC,
    //       but it could help reduce code flag
    // NOTE: should this insn have a jump offset built in?
    // - no, for consistency, let jz/jnz handle that
    // Test flag bits (logical and) with a constant
    // This can be used for tag bit tests (e.g. fixnum test)
    // Do we want to test just one specific bit, bit_idx:u8?
    // test_bit_z <bit_idx:u8>
    // test_bit_nz <bit_idx:u8>

    // TODO: we should probably have 8-bit offset versions of jump insns
    // However, this can wait. Premature optimization.
    // jmp_8, jz_8, jnz_8

    // Jump to pc offset
    // jmp <offset:i32>
    jmp,

    // Jump to pc offset if stack top is zero
    // jz <offset:i32>
    jz,

    // Jump to pc offset if stack top is not zero
    // jnz <offset:i32>
    jnz,

    // Call a function using the call stack
    // call <offset:i32> <num_args:u8> (arg0, arg1, ..., argN)
    call,

    // Call a function pointer passed as argument
    // call <num_args:u8> (arg0, arg1, ..., argN, f_ptr)
    call_fp,

    // Call into a host function
    // For example, to set up a device or to allocate more memory
    // syscall <syscall_idx:u16> (arg0, arg1, ..., argN)
    syscall,

    // Return to caller function, or
    // Return to the UVM event loop without terminating execution
    // ret (value)
    ret,

    // Terminate program execution
    // This stops the UVM event loop
    // exit (value)
    exit,

    // NOTE: last opcode must have value < 255
    // Currently, every opcode is just one byte long,
    // and we hope to keep it that way, but the value
    // 255 is reserved for future 16-bit opcode extensions.
    OP_EXT = 255,
}

#[derive(Copy, Clone, Debug, PartialEq)]
pub struct Value(u64);

impl Value
{
    pub fn is_null(&self) -> bool {
        let Value(val) = *self;
        val == 0
    }

    pub fn as_u8(&self) -> u8 {
        let Value(val) = *self;
        val as u8
    }

    pub fn as_u16(&self) -> u16 {
        let Value(val) = *self;
        val as u16
    }

    pub fn as_u32(&self) -> u32 {
        let Value(val) = *self;
        val as u32
    }

    pub fn as_u64(&self) -> u64 {
        let Value(val) = *self;
        val as u64
    }

    pub fn as_usize(&self) -> usize {
        let Value(val) = *self;
        val as usize
    }

    pub fn as_i8(&self) -> i8 {
        let Value(val) = *self;
        val as i8
    }

    pub fn as_i16(&self) -> i16 {
        let Value(val) = *self;
        val as i16
    }

    pub fn as_i32(&self) -> i32 {
        let Value(val) = *self;
        val as i32
    }

    pub fn as_i64(&self) -> i64 {
        let Value(val) = *self;
        val as i64
    }

    pub fn as_f32(&self) -> f32 {
        let Value(val) = *self;
        let val = val as i32;
        unsafe { transmute(val) }
    }
}

impl From<bool> for Value {
    fn from(val: bool) -> Self {
        Value(if val { 1 } else { 0 })
    }
}

impl From<u8> for Value {
    fn from(val: u8) -> Self {
        Value(val as u64)
    }
}

impl From<u16> for Value {
    fn from(val: u16) -> Self {
        Value(val as u64)
    }
}

impl From<u32> for Value {
    fn from(val: u32) -> Self {
        Value(val as u64)
    }
}

impl From<u64> for Value {
    fn from(val: u64) -> Self {
        Value(val as u64)
    }
}

impl From<usize> for Value {
    fn from(val: usize) -> Self {
        Value(val as u64)
    }
}

impl From<i8> for Value {
    fn from(val: i8) -> Self {
        Value((val as i64) as u64)
    }
}

impl From<i32> for Value {
    fn from(val: i32) -> Self {
        Value(val as u64)
    }
}

impl From<i64> for Value {
    fn from(val: i64) -> Self {
        Value(val as u64)
    }
}

impl From<f32> for Value {
    fn from(val: f32) -> Self {
        let val: u32 = unsafe { transmute(val) };
        Value(val as u64)
    }
}

pub struct MemBlock
{
    data: Vec<u8>
}

impl MemBlock
{
    pub fn new() -> Self
    {
        Self {
            data: Vec::default()
        }
    }

    /// Get the memory block size in bytes
    pub fn len(&self) -> usize
    {
        self.data.len()
    }

    /// Resize to a new size in bytes
    pub fn resize(&mut self, mut num_bytes: usize) -> usize
    {
        // Round up to a page size multiple
        let page_size = unsafe { libc::sysconf(libc::_SC_PAGESIZE) } as usize;
        assert!(page_size % 8 == 0);
        let rem = num_bytes % page_size;
        if rem != 0 {
            num_bytes += page_size - rem;
        }

        assert!(num_bytes % page_size == 0);
        self.data.resize(num_bytes, 0);

        num_bytes
    }

    pub fn push_op(&mut self, op: Op)
    {
        self.data.push(op as u8);
    }

    pub fn push_u8(&mut self, val: u8)
    {
        self.data.push(val);
    }

    pub fn push_u16(&mut self, val: u16)
    {
        for byte in val.to_le_bytes() {
            self.data.push(byte);
        }
    }

    pub fn push_i8(&mut self, val: i8)
    {
        self.data.push(val as u8);
    }

    pub fn push_i32(&mut self, val: i32)
    {
        for byte in val.to_le_bytes() {
            self.data.push(byte);
        }
    }

    pub fn push_u32(&mut self, val: u32)
    {
        for byte in val.to_le_bytes() {
            self.data.push(byte);
        }
    }

    pub fn push_u64(&mut self, val: u64)
    {
        for byte in val.to_le_bytes() {
            self.data.push(byte);
        }
    }

    /// Write a value at the given address
    pub fn write<T>(&mut self, pos: usize, val: T) where T: Copy
    {
        unsafe {
            let buf_ptr = self.data.as_mut_ptr();
            let val_ptr = transmute::<*mut u8 , *mut T>(buf_ptr.add(pos));
            std::ptr::write_unaligned(val_ptr, val);
        }
    }

    /// Read a value at the current PC and then increment the PC
    pub fn read_pc<T>(&self, pc: &mut usize) -> T where T: Copy
    {
        unsafe {
            let buf_ptr = self.data.as_ptr();
            let val_ptr = transmute::<*const u8 , *const T>(buf_ptr.add(*pc));
            *pc += size_of::<T>();
            std::ptr::read_unaligned(val_ptr)
        }
    }
}

struct StackFrame
{
    // Previous base pointer at the time of call
    prev_bp: usize,

    // Return address
    ret_addr: usize,

    // Argument count
    argc: usize,
}





pub struct Thread
{
    // Thread id
    pub id: u64,

    // Parent VM
    pub vm: Arc<Mutex<VM>>,

    // Value stack
    stack: Vec<Value>,

    // List of stack frames (activation records)
    frames: Vec<StackFrame>,
}

impl Thread
{
    pub fn new(tid: u64, vm: Arc<Mutex<VM>>) -> Self
    {
        Self {
            id: tid,
            vm,
            stack: Vec::default(),
            frames: Vec::default(),
        }
    }

    pub fn stack_size(&self) -> usize
    {
        self.stack.len()
    }

    pub fn push<T>(&mut self, val: T) where Value: From<T>
    {
        self.stack.push(Value::from(val));
    }

    pub fn pop(&mut self) -> Value
    {
        match self.stack.pop() {
            Some(val) => val,
            None => panic!("tried to pop when the stack is empty")
        }
    }

    /// Get the current size of the heap in bytes
    pub fn heap_size(&self) -> usize
    {
        todo!();

        //self.heap.len()
    }

    /// Get a pointer to an address/offset in the heap
    pub fn get_heap_ptr<T>(&mut self, addr: usize, num_elems: usize) -> *mut T
    {
        todo!();

        /*
        if addr + std::mem::size_of::<T>() * num_elems > self.heap.len() {
            panic!("attempting to access memory slice past end of heap");
        }

        if addr & (size_of::<T>() - 1) != 0 {
            panic!(
                "attempting to access data of type {} at unaligned address",
                std::any::type_name::<T>()
            );
        }

        unsafe {
            let heap_ptr: *mut u8 = self.heap.data.as_mut_ptr().add(addr);
            transmute::<*mut u8 , *mut T>(heap_ptr)
        }
        */
    }

    /// Get a mutable slice to access a memory region in the heap
    pub fn get_heap_slice<T>(&mut self, addr: usize, num_elems: usize) -> &mut [T]
    {
        todo!();

        /*
        if addr + std::mem::size_of::<T>() * num_elems > self.heap.len() {
            panic!("attempting to access memory slice past end of heap");
        }

        if addr & (size_of::<T>() - 1) != 0 {
            panic!(
                "attempting to access unaligned memory slice of type {}",
                std::any::type_name::<T>()
            );
        }

        unsafe {
            let heap_ptr: *mut u8 = self.heap.data.as_mut_ptr().add(addr);
            let start_ptr = transmute::<*mut u8 , *mut T>(heap_ptr);
            std::slice::from_raw_parts_mut(start_ptr, num_elems)
        }
        */
    }

    /// Copy an UTF-8 string at a given address in the heap
    pub fn get_heap_str(&mut self, str_ptr: usize) -> &str
    {
        todo!();

        /*
        // Verify that there is a null-terminator for this string
        // within the bounds of the heap
        let mut str_len = 0;
        loop
        {
            let char_idx = str_ptr + str_len;
            if char_idx >= self.heap.len() {
                panic!("string is not properly null-terminated");
            }

            if self.heap.data[char_idx] == 0 {
                break;
            }

            str_len += 1;
        }

        // Convert the string to a Rust string
        let char_ptr = self.get_heap_ptr(str_ptr, str_len);
        let c_str = unsafe { CStr::from_ptr(char_ptr as *const i8) };
        let rust_str = c_str.to_str().unwrap();
        rust_str
        */
    }

    /// Call a function at a given address
    pub fn call(&mut self, callee_pc: u64, args: Vec<Value>) -> Value
    {
        assert!(self.stack.len() == 0);
        assert!(self.frames.len() == 0);

        // Push a new stack frame
        self.frames.push(StackFrame {
            prev_bp: usize::MAX,
            ret_addr: usize::MAX,
            argc: args.len(),
        });

        // Push the arguments on the stack
        for arg in args {
            self.stack.push(arg);
        }

        // The base pointer will point at the first local
        let mut bp = self.stack.len();
        let mut pc = callee_pc as usize;

        /*
        // For each instruction to execute
        loop
        {
            #[cfg(feature = "count_insns")]
            {
                self.insn_count += 1;
            }

            if pc >= self.code.len() {
                panic!("pc outside bounds of code space")
            }

            let op = self.code.read_pc::<Op>(&mut pc);
            //dbg!(op);

            match op
            {
                Op::panic => panic!("execution error, encountered panic opcode"),

                Op::nop => continue,

                Op::pop => {
                    self.pop();
                }

                Op::getn => {
                    let n = self.code.read_pc::<u8>(&mut pc) as usize;
                    let val = self.stack[self.stack.len() - (1 + n)];
                    self.push(val);
                }

                Op::setn => {
                    let n = self.code.read_pc::<u8>(&mut pc) as usize;
                    let val = self.pop();
                    let len = self.stack.len();
                    self.stack[len - (1 + n)] = val;
                }

                Op::dup => {
                    let val = self.pop();
                    self.push(val);
                    self.push(val);
                }

                Op::swap => {
                    let a = self.pop();
                    let b = self.pop();
                    self.push(a);
                    self.push(b);
                }

                Op::get_arg => {
                    let idx = self.code.read_pc::<u8>(&mut pc) as usize;

                    let argc = self.frames[self.frames.len() - 1].argc;
                    if idx >= argc {
                        panic!("invalid index in get_arg, idx={}, argc={}", idx, argc);
                    }

                    // Last argument is at bp - 1 (if there are arguments)
                    let stack_idx = (bp - argc) + idx;
                    self.push(self.stack[stack_idx]);
                }

                Op::get_var_arg => {
                    let idx = self.pop().as_usize();

                    let argc = self.frames[self.frames.len() - 1].argc;
                    if idx >= argc {
                        panic!("invalid index in get_arg, idx={}, argc={}", idx, argc);
                    }

                    // Last argument is at bp - 1 (if there are arguments)
                    let stack_idx = (bp - argc) + idx;
                    self.push(self.stack[stack_idx]);
                }

                Op::set_arg => {
                    let idx = self.code.read_pc::<u8>(&mut pc) as usize;

                    let argc = self.frames[self.frames.len() - 1].argc;
                    if idx >= argc {
                        panic!("invalid index in set_arg, idx={}, argc={}", idx, argc);
                    }

                    // Last argument is at bp - 1 (if there are arguments)
                    let stack_idx = (bp - argc) + idx;
                    let val = self.pop();
                    self.stack[stack_idx] = val;
                }

                Op::get_local => {
                    let idx = self.code.read_pc::<u8>(&mut pc) as usize;

                    if bp + idx >= self.stack.len() {
                        panic!("invalid index {} in get_local", idx);
                    }

                    self.push(self.stack[bp + idx]);
                }

                Op::set_local => {
                    let idx = self.code.read_pc::<u8>(&mut pc) as usize;
                    let val = self.pop();

                    if bp + idx >= self.stack.len() {
                        panic!("invalid index in set_local");
                    }

                    self.stack[bp + idx] = val;
                }

                Op::push_0 => {
                    self.push(0);
                }
                Op::push_1 => {
                    self.push(1);
                }
                Op::push_2 => {
                    self.push(2);
                }

                Op::push_0n => {
                    let n = self.code.read_pc::<u8>(&mut pc);
                    self.stack.resize(self.stack.len() + n as usize, Value::from(0));
                }

                Op::push_i8 => {
                    let val = self.code.read_pc::<i8>(&mut pc);
                    self.push(val);
                }

                Op::push_u32 => {
                    let val = self.code.read_pc::<u32>(&mut pc);
                    self.push(val);
                }

                Op::push_u64 => {
                    let val = self.code.read_pc::<u64>(&mut pc);
                    self.push(val);
                }

                Op::and_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u32() & v1.as_u32());
                }

                Op::or_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u32() | v1.as_u32());
                }

                Op::xor_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u32() ^ v1.as_u32());
                }

                Op::not_u32 => {
                    let v0 = self.pop();
                    self.push(!v0.as_u32());
                }

                Op::lshift_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u32().wrapping_shl(v1.as_u32())
                    );
                }

                Op::rshift_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u32().wrapping_shr(v1.as_u32())
                    );
                }

                Op::rshift_i32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_i32().wrapping_shr(v1.as_u32())
                    );
                }

                Op::add_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u32().wrapping_add(v1.as_u32())
                    );
                }

                Op::sub_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u32().wrapping_sub(v1.as_u32())
                    );
                }

                Op::mul_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u32().wrapping_mul(v1.as_u32())
                    );
                }

                // Division by zero will cause a panic (this is intentional)
                Op::div_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u32() / v1.as_u32()
                    );
                }

                // Division by zero will cause a panic (this is intentional)
                Op::mod_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u32() % v1.as_u32()
                    );
                }

                // Division by zero will cause a panic (this is intentional)
                Op::div_i32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_i32() / v1.as_i32()
                    );
                }

                // Division by zero will cause a panic (this is intentional)
                Op::mod_i32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_i32() % v1.as_i32()
                    );
                }

                Op::eq_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u32() == v1.as_u32());
                }

                Op::ne_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u32() != v1.as_u32());
                }

                Op::lt_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u32() < v1.as_u32());
                }

                Op::le_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u32() <= v1.as_u32());
                }

                Op::gt_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u32() > v1.as_u32());
                }

                Op::ge_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u32() >= v1.as_u32());
                }

                Op::lt_i32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_i32() < v1.as_i32());
                }

                Op::le_i32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_i32() <= v1.as_i32());
                }

                Op::gt_i32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_i32() > v1.as_i32());
                }

                Op::ge_i32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_i32() >= v1.as_i32());
                }

                Op::and_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u64() & v1.as_u64());
                }

                Op::or_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u64() | v1.as_u64());
                }

                Op::xor_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u64() ^ v1.as_u64());
                }

                Op::not_u64 => {
                    let v0 = self.pop();
                    self.push(!v0.as_u64());
                }

                Op::lshift_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u64().wrapping_shl(v1.as_u32())
                    );
                }

                Op::rshift_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u64().wrapping_shr(v1.as_u32())
                    );
                }

                Op::rshift_i64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_i64().wrapping_shr(v1.as_u32())
                    );
                }

                Op::add_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u64().wrapping_add(v1.as_u64())
                    );
                }

                Op::sub_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u64().wrapping_sub(v1.as_u64())
                    );
                }

                Op::mul_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u64().wrapping_mul(v1.as_u64())
                    );
                }

                // Division by zero will cause a panic (this is intentional)
                Op::div_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u64() / v1.as_u64()
                    );
                }

                // Division by zero will cause a panic (this is intentional)
                Op::mod_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u64() % v1.as_u64()
                    );
                }

                // Division by zero will cause a panic (this is intentional)
                Op::div_i64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_i64() / v1.as_i64()
                    );
                }

                // Division by zero will cause a panic (this is intentional)
                Op::mod_i64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_i64() % v1.as_i64()
                    );
                }

                Op::eq_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u64() == v1.as_u64());
                }

                Op::ne_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u64() != v1.as_u64());
                }

                Op::lt_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u64() < v1.as_u64());
                }

                Op::le_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u64() <= v1.as_u64());
                }

                Op::gt_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u64() > v1.as_u64());
                }

                Op::ge_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u64() >= v1.as_u64());
                }

                Op::lt_i64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_i64() < v1.as_i64());
                }

                Op::le_i64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_i64() <= v1.as_i64());
                }

                Op::gt_i64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_i64() > v1.as_i64());
                }

                Op::ge_i64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_i64() >= v1.as_i64());
                }

                Op::sx_i8_i32 => {
                    let v = self.pop();
                    self.push(v.as_i8() as i32);
                }

                Op::sx_i8_i64 => {
                    let v = self.pop();
                    self.push(v.as_i8() as i64);
                }

                Op::sx_i16_i32 => {
                    let v = self.pop();
                    self.push(v.as_i16() as i32);
                }

                Op::sx_i16_i64 => {
                    let v = self.pop();
                    self.push(v.as_i16() as i64);
                }

                Op::sx_i32_i64 => {
                    let v = self.pop();
                    self.push(v.as_i32() as i64);
                }

                Op::trunc_u8 => {
                    let v = self.pop();
                    self.push(v.as_u8());
                }

                Op::trunc_u16 => {
                    let v = self.pop();
                    self.push(v.as_u16());
                }

                Op::trunc_u32 => {
                    let v = self.pop();
                    self.push(v.as_u32());
                }

                Op::add_f32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_f32() + v1.as_f32());
                }

                Op::sub_f32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_f32() - v1.as_f32());
                }

                Op::mul_f32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_f32() * v1.as_f32());
                }

                // Should return NaN for invalid inputs
                Op::div_f32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_f32() / v1.as_f32());
                }

                Op::sin_f32 => {
                    let v0 = self.pop().as_f32();
                    self.push(v0.sin());
                }

                Op::cos_f32 => {
                    let v0 = self.pop().as_f32();
                    self.push(v0.cos());
                }

                // Should return NaN for invalid inputs
                Op::tan_f32 => {
                    let v0 = self.pop().as_f32();
                    self.push(v0.tan());
                }

                // Should return NaN for invalid inputs
                Op::asin_f32 => {
                    let v0 = self.pop().as_f32();
                    self.push(v0.asin());
                }

                // Should return NaN for invalid inputs
                Op::acos_f32 => {
                    let v0 = self.pop().as_f32();
                    self.push(v0.acos());
                }

                Op::atan_f32 => {
                    let v0 = self.pop().as_f32();
                    self.push(v0.atan());
                }

                // Should return NaN for invalid inputs
                Op::pow_f32 => {
                    let v1 = self.pop().as_f32();
                    let v0 = self.pop().as_f32();
                    self.push(v0.powf(v1));
                }

                // Should return NaN for invalid inputs
                Op::sqrt_f32 => {
                    let v0 = self.pop().as_f32();
                    self.push(v0.sqrt());
                }

                Op::eq_f32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_f32() == v1.as_f32());
                }

                Op::ne_f32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_f32() != v1.as_f32());
                }

                Op::lt_f32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_f32() < v1.as_f32());
                }

                Op::le_f32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_f32() <= v1.as_f32());
                }

                Op::gt_f32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_f32() > v1.as_f32());
                }

                Op::ge_f32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_f32() >= v1.as_f32());
                }

                // Follows Rust semantics:
                // - Round ties to even
                // - Never panics
                Op::i32_to_f32 => {
                    let v = self.pop();
                    self.push(v.as_i32() as f32);
                }

                // Follows Rust semantics:
                // - Round ties to even
                // - Never panics
                Op::i64_to_f32 => {
                    let v = self.pop();
                    self.push(v.as_i64() as f32);
                }

                // Follows Rust semantics:
                // - Rounds towards zero (truncates)
                // - Saturates to min/max int values
                // - NaN converts to zero
                // - Never panics
                Op::f32_to_i32 => {
                    let v = self.pop();
                    self.push(v.as_f32() as i32);
                }

                Op::load_u8 => {
                    let addr = self.pop().as_usize();
                    let heap_ptr = self.get_heap_ptr(addr, 1);
                    let val: u8 = unsafe { *heap_ptr };
                    self.push(val);
                }

                Op::load_u16 => {
                    let addr = self.pop().as_usize();
                    let heap_ptr = self.get_heap_ptr(addr, 1);
                    let val: u16 = unsafe { *heap_ptr };
                    self.push(val);
                }

                Op::load_u32 => {
                    let addr = self.pop().as_usize();
                    let heap_ptr = self.get_heap_ptr(addr, 1);
                    let val: u32 = unsafe { *heap_ptr };
                    self.push(val);
                }

                Op::load_u64 => {
                    let addr = self.pop().as_usize();
                    let heap_ptr = self.get_heap_ptr(addr, 1);
                    let val: u64 = unsafe { *heap_ptr };
                    self.push(val);
                }

                Op::store_u8 => {
                    let val = self.pop().as_u8();
                    let addr = self.pop().as_usize();
                    let heap_ptr = self.get_heap_ptr(addr, 1);
                    unsafe { *heap_ptr = val; }
                }

                Op::store_u16 => {
                    let val = self.pop().as_u16();
                    let addr = self.pop().as_usize();
                    let heap_ptr = self.get_heap_ptr(addr, 1);
                    unsafe { *heap_ptr = val; }
                }

                Op::store_u32 => {
                    let val = self.pop().as_u32();
                    let addr = self.pop().as_usize();
                    let heap_ptr = self.get_heap_ptr(addr, 1);
                    unsafe { *heap_ptr = val; }
                }

                Op::store_u64 => {
                    let val = self.pop().as_u64();
                    let addr = self.pop().as_usize();
                    let heap_ptr = self.get_heap_ptr(addr, 1);
                    unsafe { *heap_ptr = val; }
                }

                Op::jmp => {
                    let offset = self.code.read_pc::<i32>(&mut pc) as isize;
                    pc = ((pc as isize) + offset) as usize;
                }

                Op::jz => {
                    let offset = self.code.read_pc::<i32>(&mut pc) as isize;
                    let v0 = self.pop();

                    if v0.as_i64() == 0 {
                        pc = ((pc as isize) + offset) as usize;
                    }
                }

                Op::jnz => {
                    let offset = self.code.read_pc::<i32>(&mut pc) as isize;
                    let v0 = self.pop();

                    if v0.as_i64() != 0 {
                        pc = ((pc as isize) + offset) as usize;
                    }
                }

                // call <num_args:u8> <offset:i32> (arg0, arg1, ..., argN)
                Op::call => {
                    // Offset of the function to call
                    let offset = self.code.read_pc::<i32>(&mut pc) as isize;

                    // Argument count
                    let num_args = self.code.read_pc::<u8>(&mut pc) as usize;
                    assert!(num_args <= self.stack.len() - bp);

                    self.frames.push(StackFrame {
                        prev_bp: bp,
                        ret_addr: pc,
                        argc: num_args,
                    });

                    // The base pointer will point at the first local
                    bp = self.stack.len();
                    pc = ((pc as isize) + offset) as usize;
                }

                // call <num_args:u8> (arg0, arg1, ..., argN, f_ptr)
                Op::call_fp => {
                    // Absolute address of the function to call
                    let fp = self.pop();

                    // Argument count
                    let num_args = self.code.read_pc::<u8>(&mut pc) as usize;
                    assert!(num_args <= self.stack.len() - bp);

                    self.frames.push(StackFrame {
                        prev_bp: bp,
                        ret_addr: pc,
                        argc: num_args,
                    });

                    // The base pointer will point at the first local
                    bp = self.stack.len();
                    pc = fp.as_usize();
                }

                Op::syscall => {
                    let syscall_idx = self.code.read_pc::<u16>(&mut pc);
                    let syscall_fn = get_syscall(syscall_idx);

                    todo!();

                    /*
                    match syscall_fn
                    {
                        SysCallFn::Fn0_0(fun) => {
                            fun(self)
                        }

                        SysCallFn::Fn0_1(fun) => {
                            let v = fun(self);
                            self.push(v);
                        }

                        SysCallFn::Fn1_0(fun) => {
                            let a0 = self.pop();
                            fun(self, a0)
                        }

                        SysCallFn::Fn1_1(fun) => {
                            let a0 = self.pop();
                            let v = fun(self, a0);
                            self.push(v);
                        }

                        SysCallFn::Fn2_0(fun) => {
                            let a1 = self.pop();
                            let a0 = self.pop();
                            fun(self, a0, a1)
                        }

                        SysCallFn::Fn2_1(fun) => {
                            let a1 = self.pop();
                            let a0 = self.pop();
                            let v = fun(self, a0, a1);
                            self.push(v);
                        }

                        SysCallFn::Fn3_0(fun) => {
                            let a2 = self.pop();
                            let a1 = self.pop();
                            let a0 = self.pop();
                            fun(self, a0, a1, a2)
                        }

                        SysCallFn::Fn3_1(fun) => {
                            let a2 = self.pop();
                            let a1 = self.pop();
                            let a0 = self.pop();
                            let v = fun(self, a0, a1, a2);
                            self.push(v);
                        }

                        SysCallFn::Fn4_0(fun) => {
                            let a3 = self.pop();
                            let a2 = self.pop();
                            let a1 = self.pop();
                            let a0 = self.pop();
                            fun(self, a0, a1, a2, a3)
                        }

                        SysCallFn::Fn4_1(fun) => {
                            let a3 = self.pop();
                            let a2 = self.pop();
                            let a1 = self.pop();
                            let a0 = self.pop();
                            let v = fun(self, a0, a1, a2, a3);
                            self.push(v);
                        }
                    }
                    */
                }

                Op::exit => {
                    if self.stack.len() <= bp {
                        panic!("exit with no return value on stack");
                    }

                    let val = self.pop();
                    self.stack.clear();
                    self.frames.clear();
                    return val;
                }

                Op::ret => {
                    if self.stack.len() <= bp {
                        panic!("ret with no return value on stack");
                    }

                    let ret_val = self.pop();

                    // If this is a top-level return
                    if self.frames.len() == 1 {
                        self.stack.clear();
                        self.frames.clear();
                        return ret_val;
                    }

                    assert!(self.frames.len() > 0);
                    let top_frame = self.frames.pop().unwrap();

                    // Pop all local variables and arguments
                    // We pop arguments in the callee so we can support tail calls
                    assert!(self.stack.len() >= bp - top_frame.argc);
                    self.stack.truncate(bp - top_frame.argc);

                    pc = top_frame.ret_addr;
                    bp = top_frame.prev_bp;

                    self.push(ret_val);
                }

                _ => panic!("unknown opcode {:?}", op),
            }
        }
        */

        todo!();
    }



}







pub struct VM
{
    // Heap memory space
    heap: MemBlock,

    // Code memory space
    code: MemBlock,

    // Next thread id to assign
    next_tid: u64,

    // Map from actor ids to thread join handles
    threads: HashMap<u64, thread::JoinHandle<Value>>,

    // Reference to self
    // Needed to instantiate actors
    vm: Option<Arc<Mutex<VM>>>,
}

// Needed to send Arc<Mutex<VM>> to thread
unsafe impl Send for VM {}

impl VM
{
    pub fn new(mut code: MemBlock, mut heap: MemBlock, syscalls: HashSet<u16>) -> Arc<Mutex<VM>>
    {
        // Resize the code and heap space to a page size multiple
        code.resize(code.len());
        heap.resize(heap.len());

        let vm = Self {
            code,
            heap,
            next_tid: 0,
            threads: HashMap::default(),
            vm: None,
        };

        let vm = Arc::new(Mutex::new(vm));

        // Store a reference to the mutex on the VM
        // This is so we can pass this reference to threads
        vm.lock().unwrap().vm = Some(vm.clone());

        vm
    }

    /*
    /// Resize the heap to a new size in bytes
    pub fn resize_heap(&mut self, num_bytes: usize) -> usize
    {
        self.heap.resize(num_bytes)
    }
    */

    // Create a new thread
    pub fn new_thread(vm: &Arc<Mutex<VM>>, fun: Value, args: Vec<Value>) -> u64
    {
        /*
        let vm_mutex = vm.clone();

        // Assign an actor id
        let mut vm_ref = vm.lock().unwrap();
        let actor_id = vm_ref.next_actor_id;
        vm_ref.next_actor_id += 1;
        drop(vm_ref);

        // Create a message queue for the actor
        let (queue_tx, queue_rx) = mpsc::channel::<Message>();

        // Spawn a new thread for the actor
        let handle = thread::spawn(move || {
            let mut actor = Actor::new(actor_id, vm_mutex, queue_rx);
            actor.call(fun, &args)
        });

        // Store the join handles and queue endpoints on the VM
        let mut vm_ref = vm.lock().unwrap();
        vm_ref.threads.insert(actor_id, handle);
        vm_ref.actor_txs.insert(actor_id, queue_tx);
        drop(vm_ref);

        tid
        */

        todo!();
    }

    // Wait for a thread to produce a result and return it
    pub fn join_thread(vm: &Arc<Mutex<VM>>, tid: u64) -> Value
    {
        /*
        // Get the join handle, then release the VM lock
        let mut vm = vm.lock().unwrap();
        let mut handle = vm.threads.remove(&tid).unwrap();
        drop(vm);

        // Note: there is no need to copy data when joining,
        // because the actor sending the data is done running
        handle.join().expect(&format!("could not actor thread with id {}", tid))
        */

        todo!();
    }

    // Call a function in the main actor
    pub fn call(vm: &mut Arc<Mutex<VM>>, callee_pc: u64, args: Vec<Value>) -> Value
    {
        let vm_mutex = vm.clone();

        // Assign an actor id
        // Store the queue endpoints on the VM
        let mut vm_ref = vm.lock().unwrap();
        let tid = vm_ref.next_tid;
        assert!(tid == 0);
        vm_ref.next_tid += 1;
        drop(vm_ref);

        let mut thread = Thread::new(tid, vm_mutex);
        thread.call(callee_pc, args)
    }
}

#[cfg(test)]
mod tests
{
    use super::*;
    use crate::asm::*;

    fn eval_src(src: &str) -> Value
    {
        dbg!(src);
        let asm = Assembler::new();
        let mut vm = asm.parse_str(src).unwrap();
        let result = vm.call(0, &[]);
        assert!(vm.stack.len() == 0 && vm.frames.len() == 0);

        result
    }

    fn eval_i64(src: &str, expected: i64)
    {
        let result = eval_src(src);
        assert_eq!(result, expected.into());
    }

    #[test]
    fn test_opcodes()
    {
        // We can have at most 254 short single-byte opcodes
        assert!(Op::exit as usize <= 254);

        // Keep track of how many short opcodes we have so far
        dbg!(Op::exit as usize);
        assert!(Op::exit as usize <= 113);
    }

    #[test]
    fn test_basics()
    {
        // Integer literals
        eval_i64("push_i8 1; exit;", 1);
        eval_i64("push_i8 -3; exit;", -3);
        eval_i64("push_u64 1_333_444; exit;", 1_333_444);
        eval_i64("push_u64 0xFF; exit;", 0xFF);
        eval_i64("push_u64 0b1101; exit;", 0b1101);

        // Push mnemonic
        eval_i64("push 0; exit;", 0);
        eval_i64("push 1; exit;", 1);
        eval_i64("push -1; exit;", -1);
        eval_i64("push 0xFFFF; exit;", 0xFFFF);
        eval_i64(".data; LABEL: .u64 0; .code; push LABEL; exit;", 0);

        // Stack manipulation
        eval_i64("push_i8 7; push_i8 3; swap; exit;", 7);
        eval_i64("push_i8 7; push_i8 3; swap; swap; pop; exit;", 7);

        // Integer arithmetic
        eval_i64("push_i8 1; push_i8 10; add_u64; exit;", 11);
        eval_i64("push_i8 5; push_i8 10; sub_u64; exit;", -5);
        eval_i64("push_i8 10; push_i8 2; sub_u64; exit;", 8);
        eval_i64("push 5; push_i8 -6; mul_u64; exit;", -30);
        eval_i64("push 1; push 2; lshift_u64; exit;", 4);

        // Comparisons
        eval_i64("push_i8 1; push_i8 10; lt_i64; exit;", 1);
        eval_i64("push_i8 11; push_i8 1; lt_i64; exit;", 0);
    }

    #[test]
    fn test_setlocal()
    {
        eval_i64(".code; push 0; push 77; set_local 0; get_local 0; exit;", 77);
    }

    #[test]
    fn test_floats()
    {
        eval_i64("push_f32 1.5; push_f32 2.5; add_f32; push_f32 4.0; eq_u64; exit;", 1);
    }

    #[test]
    fn test_loop()
    {
        // Simple loop
        eval_i64("push_i8 0; LOOP: push_i8 1; add_u64; dup; push_i8 10; eq_u64; jz LOOP; exit;", 10);
    }

    #[test]
    fn test_load_store()
    {
        // Store instruction
        eval_i64(".data; .zero 255; .code; push_i8 0; push_i8 77; store_u8; push_i8 11; exit;", 11);
    }

    #[test]
    fn test_setn()
    {
        // Store instruction
        eval_i64(".code; push 3; push 0; push 7; setn 1; pop; exit;", 7);
    }

    #[test]
    fn test_call_ret()
    {
        eval_i64("call FN, 0; exit; FN: push_i8 33; ret;", 33);
        eval_i64("push_i8 3; call FN, 1; exit; FN: get_arg 0; push_i8 1; add_u64; ret;", 4);

        // set_arg
        eval_i64("push_i8 3; call FN, 1; exit; FN: push 7; set_arg 0; get_arg 0; ret;", 7);

        // Two arguments and subtract (order of arguments matters)
        eval_i64("push_i8 7; push 5; call FN, 2; exit; FN: get_arg 0; get_arg 1; sub_u64; ret;", 2);

        // Recursive decrement function
        eval_i64("push 10; call DEC, 1; exit; DEC: get_arg 0; dup; jz ZERO; push 1; sub_u64; call DEC, 1; ret; ZERO: ret;", 0);

        // Regression: stack corruption
        eval_i64("push 5; call foo, 0; pop; exit; foo: push 2; push 0; ret;", 5);
    }

    #[test]
    fn test_call_fp()
    {
        eval_i64(" push FN; call_fp 0; exit; FN: push_i8 33; ret;", 33);
    }

    #[test]
    fn test_syscalls()
    {
        eval_src(".data; LABEL: .zero 256; .code; push LABEL; push 255; push 0; syscall memset; push 0; exit;");
    }

    #[test]
    #[should_panic]
    fn test_div_zero()
    {
        eval_src("push 8; push 0; div_u64; exit;");
    }

    #[test]
    #[should_panic]
    fn test_ret_none()
    {
        eval_src("call FN, 0; exit; FN: ret;");
    }

    #[test]
    #[should_panic]
    fn test_get_arg_none()
    {
        eval_src("call FN, 0; exit; FN: get_arg 0; push 0; ret;");
    }

    #[test]
    #[should_panic]
    fn test_load_oob()
    {
        eval_src(".data; .fill 1000, 0; .code; push 100_000_000; load_u64; exit;");
    }

    #[test]
    #[should_panic]
    fn test_memset_oob()
    {
        eval_src(".data; LABEL: .zero 1; .code; push LABEL; push 255; push 100_000_000; syscall memset; push 0; exit;");
    }

    // Regression: this used to segfault
    #[test]
    #[should_panic]
    fn test_memcmp_n1()
    {
        eval_src(".data; A: .zero 10; B: .zero 10; .code; push A; push B; push -1; syscall memcpy;");
    }
}
