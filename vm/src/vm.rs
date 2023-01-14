use std::mem::{transmute, size_of};
use std::ffi::CStr;
use crate::syscalls::*;

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

    // Push the value zero
    push_0,

    // push_i8 <i8_imm> (sign-extended)
    push_i8,

    // push_u32 <u32_imm>
    push_u32,

    // push_u64 <u64_imm>
    push_u64,

    // Stack manipulation
    pop,
    dup,
    swap,

    // Copy the nth-value from the top of the stack
    // getn 0 is equivalent to dup
    // getn <idx:u8>
    getn,

    // Pop N values off the stack
    // popn <n:u8>
    popn,

    // Get the argument count for the current stack frame
    get_argc,

    // Get the function argument at a given index
    // get_arg <idx:u8>
    get_arg,

    // Set the function argument at a given index
    // set_arg <idx:u8> (value)
    set_arg,

    // Get the local variable at a given index
    // get_local <idx:u8>
    get_local,

    // Set the local variable at a given index
    // set_local <idx:u8> (value)
    set_local,

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

    //add_u64_ovf,
    //sub_u64_ovf,
    //mul_i64_ovf, // produces two 64-bit words of output


    // NOTE: may want to wait for this because it's not RISC,
    //       but it could help reduce code flag
    // NOTE: should this insn have a jump offset built in?
    // Test flag bits (logical and) with a constant
    // This can be used for tag bit tests
    // Do we want to test just one specific bit, bit_idx:u8?
    // test_u8 <u8_flags>



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
    /// Load from heap at fixed address
    /// This is used for reading global variables
    /// The address is multiplied by the data size (x 4 or x8)
    /// If we save 24 bits for the offset, then that gives us quite a lot
    load_static_u64 <address>
    */

    // Jump to pc offset
    jmp,

    // Jump to pc offset if stack top is zero
    jz,

    // Jump to pc offset if stack top is not zero
    jnz,

    // Call a function using the call stack
    // call <offset:i32> <num_args:u8> (arg0, arg1, ..., argN)
    call,

    // Call a function pointer passed as argument
    // call <num_args:u8> (f_ptr, arg0, arg1, ..., argN)
    call_fp,

    // Call into a blocking host function
    // For example, to set up a device or to allocate more memory
    // syscall <syscall_idx:u16> (arg0, arg1, ..., argN)
    syscall,

    // Return to caller function
    // ret (value)
    ret,

    // End execution normally
    // exit (value)
    exit,

    // NOTE: last opcode must have value < 255
    // The value 255 is reserved for 16-bit opcode extensions.
    // OP_EXT = 255
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

    pub fn as_i32(&self) -> i32 {
        let Value(val) = *self;
        val as i32
    }

    pub fn as_i64(&self) -> i64 {
        let Value(val) = *self;
        val as i64
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

    /// Read a value at the current PC and then increment the PC
    pub fn write<T>(&mut self, pos: usize, val: T) where T: Copy
    {
        unsafe {
            let buf_ptr = self.data.as_mut_ptr();
            let val_ptr = transmute::<*mut u8 , *mut T>(buf_ptr.add(pos));
            *val_ptr = val;
        }
    }

    /// Read a value at the current PC and then increment the PC
    pub fn read_pc<T>(&self, pc: &mut usize) -> T where T: Copy
    {
        unsafe {
            let buf_ptr = self.data.as_ptr();
            let val_ptr = transmute::<*const u8 , *const T>(buf_ptr.add(*pc));
            *pc += size_of::<T>();
            *val_ptr
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

pub enum ExitReason
{
    Return(Value),
    Exit(Value),
    //Panic,
}

pub struct VM
{
    // Host system state
    pub sys_state: SysState,

    /// Table of system calls the program can refer to
    syscalls: Vec<SysCallFn>,

    // Heap memory space
    heap: MemBlock,

    // Code memory space
    code: MemBlock,

    // Value stack
    stack: Vec<Value>,

    // List of stack frames (activation records)
    frames: Vec<StackFrame>,
}

impl VM
{
    pub fn new(code: MemBlock, heap: MemBlock, syscalls: Vec<String>) -> Self
    {
        // Initialize the system state
        let sys_state = SysState::new();

        let mut syscall_fns = Vec::new();

        for syscall_name in syscalls {
            syscall_fns.push(sys_state.get_syscall(&syscall_name));
        }

        Self {
            sys_state,
            syscalls: syscall_fns,
            code,
            heap,
            stack: Vec::default(),
            frames: Vec::default(),
        }
    }

    pub fn stack_size(&self) -> usize
    {
        self.stack.len()
    }

    pub fn push(&mut self, val: Value)
    {
        self.stack.push(val);
    }

    pub fn push_bool(&mut self, val: bool)
    {
        let int_val: i64 = if val { 1 } else { 0 };
        self.stack.push(int_val.into());
    }

    pub fn pop(&mut self) -> Value
    {
        match self.stack.pop() {
            Some(val) => val,
            None => panic!("tried to pop when the stack is empty")
        }
    }

    /// Get a pointer to an address/offset in the heap
    pub fn get_heap_ptr<T>(&mut self, addr: usize) -> *mut T
    {
        if addr + std::mem::size_of::<T>() > self.heap.len() {
            panic!("attempting to access data past end of heap");
        }

        unsafe {
            let heap_ptr: *mut u8 = self.heap.data.as_mut_ptr().add(addr);
            transmute::<*mut u8 , *mut T>(heap_ptr)
        }
    }

    /// Get a mutable slice to access a memory region in the heap
    pub fn get_heap_slice<T>(&mut self, addr: usize, num_elems: usize) -> &mut [T]
    {
        if addr + std::mem::size_of::<T>() * num_elems > self.heap.len() {
            panic!("attempting to access memory slice past end of heap");
        }

        unsafe {
            let heap_ptr: *mut u8 = self.heap.data.as_mut_ptr().add(addr);

            let start_ptr = transmute::<*mut u8 , *mut T>(heap_ptr);

            std::slice::from_raw_parts_mut(start_ptr, num_elems)
        }
    }

    /// Copy an UTF-8 string at a given address in the heap
    pub fn get_heap_str(&mut self, str_ptr: usize) -> &str
    {
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
        let char_ptr = self.get_heap_ptr(str_ptr);
        let c_str = unsafe { CStr::from_ptr(char_ptr as *const i8) };
        let rust_str = c_str.to_str().unwrap();
        rust_str
    }

    /// Call a function at a given address
    pub fn call(&mut self, callee_pc: u64, args: &[Value]) -> ExitReason
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
            self.stack.push(*arg);
        }

        // The base pointer will point at the first local
        let mut bp = self.stack.len();
        let mut pc = callee_pc as usize;

        // For each instruction to execute
        loop
        {
            if pc >= self.code.len() {
                panic!("pc outside bounds of code space")
            }

            let op = self.code.read_pc::<Op>(&mut pc);
            //dbg!(op);

            match op
            {
                Op::panic => panic!("execution error, encountered panic opcode"),

                Op::nop => continue,

                Op::exit => {
                    let val = self.pop();
                    self.stack.clear();
                    self.frames.clear();
                    return ExitReason::Exit(val);
                }

                Op::pop => {
                    self.pop();
                }

                Op::popn => {
                    let n = self.code.read_pc::<u8>(&mut pc);
                    for _ in 0..n {
                        self.pop();
                    }
                }

                Op::getn => {
                    let n = self.code.read_pc::<u8>(&mut pc) as usize;
                    let val = self.stack[self.stack.len() - (1 + n)];
                    self.push(val);
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
                    self.push(Value::from(0 as u64));
                }

                Op::push_i8 => {
                    let val = self.code.read_pc::<i8>(&mut pc);
                    self.push(val.into());
                }

                Op::push_u32 => {
                    let val = self.code.read_pc::<u32>(&mut pc);
                    self.push(Value::from(val));
                }

                Op::push_u64 => {
                    let val = self.code.read_pc::<u64>(&mut pc);
                    self.push(Value::from(val));
                }

                Op::and_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(Value::from(
                        v0.as_u64() & v1.as_u64()
                    ));
                }

                Op::or_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(Value::from(
                        v0.as_u64() | v1.as_u64()
                    ));
                }

                Op::xor_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(Value::from(
                        v0.as_u64() ^ v1.as_u64()
                    ));
                }

                Op::not_u64 => {
                    let v0 = self.pop();
                    self.push(Value::from(
                        !v0.as_u64()
                    ));
                }

                Op::lshift_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(Value::from(
                        v0.as_u64() << v1.as_u64()
                    ));
                }

                Op::rshift_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(Value::from(
                        v0.as_u64() >> v1.as_u64()
                    ));
                }

                Op::add_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(Value::from(
                        v0.as_u64().wrapping_add(v1.as_u64())
                    ));
                }

                Op::sub_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(Value::from(
                        v0.as_u64().wrapping_sub(v1.as_u64())
                    ));
                }

                Op::mul_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(Value::from(
                        v0.as_u64().wrapping_mul(v1.as_u64())
                    ));
                }

                Op::div_i64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(Value::from(
                        v0.as_i64() / v1.as_i64()
                    ));
                }

                Op::mod_i64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(Value::from(
                        v0.as_i64() % v1.as_i64()
                    ));
                }

                Op::eq_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push_bool(v0.as_u64() == v1.as_u64());
                }

                Op::ne_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push_bool(v0.as_u64() != v1.as_u64());
                }

                Op::lt_i64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push_bool(v0.as_i64() < v1.as_i64());
                }

                Op::le_i64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push_bool(v0.as_i64() <= v1.as_i64());
                }

                Op::gt_i64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push_bool(v0.as_i64() > v1.as_i64());
                }

                Op::ge_i64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push_bool(v0.as_i64() >= v1.as_i64());
                }

                Op::load_u8 => {
                    let addr = self.pop().as_usize();
                    let heap_ptr = self.get_heap_ptr(addr);
                    let val: u8 = unsafe { *heap_ptr };
                    self.push(Value::from(val));
                }

                Op::load_u16 => {
                    let addr = self.pop().as_usize();
                    let heap_ptr = self.get_heap_ptr(addr);
                    let val: u16 = unsafe { *heap_ptr };
                    self.push(Value::from(val));
                }

                Op::load_u32 => {
                    let addr = self.pop().as_usize();
                    let heap_ptr = self.get_heap_ptr(addr);
                    let val: u32 = unsafe { *heap_ptr };
                    self.push(Value::from(val));
                }

                Op::load_u64 => {
                    let addr = self.pop().as_usize();
                    let heap_ptr = self.get_heap_ptr(addr);
                    let val: u64 = unsafe { *heap_ptr };
                    self.push(Value::from(val));
                }

                Op::store_u8 => {
                    let val = self.pop().as_u8();
                    let addr = self.pop().as_usize();
                    let heap_ptr = self.get_heap_ptr(addr);
                    unsafe { *heap_ptr = val; }
                }

                Op::store_u16 => {
                    let val = self.pop().as_u16();
                    let addr = self.pop().as_usize();
                    let heap_ptr = self.get_heap_ptr(addr);
                    unsafe { *heap_ptr = val; }
                }

                Op::store_u32 => {
                    let val = self.pop().as_u32();
                    let addr = self.pop().as_usize();
                    let heap_ptr = self.get_heap_ptr(addr);
                    unsafe { *heap_ptr = val; }
                }

                Op::store_u64 => {
                    let val = self.pop().as_u64();
                    let addr = self.pop().as_usize();
                    let heap_ptr = self.get_heap_ptr(addr);
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

                Op::syscall => {
                    let table_idx = self.code.read_pc::<u16>(&mut pc) as usize;

                    assert!(table_idx < self.syscalls.len());
                    let syscall_fn = self.syscalls[table_idx];

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

                        SysCallFn::Fn2_0(fun) => {
                            let a1 = self.pop();
                            let a0 = self.pop();
                            fun(self, a0, a1)
                        }

                        SysCallFn::Fn3_0(fun) => {
                            let a2 = self.pop();
                            let a1 = self.pop();
                            let a0 = self.pop();
                            fun(self, a0, a1, a2)
                        }
                    }
                }

                Op::ret => {
                    if self.stack.len() <= bp {
                        panic!("ret with no return value on stack");
                    }

                    let ret_val = self.pop();

                    if self.frames.len() == 1 {
                        self.stack.clear();
                        self.frames.clear();
                        return ExitReason::Return(ret_val);
                    }

                    assert!(self.frames.len() > 0);
                    let top_frame = self.frames.pop().unwrap();
                    pc = top_frame.ret_addr;
                    bp = top_frame.prev_bp;

                    // Pop the arguments
                    // We do this in the callee so we can support tail calls
                    for _ in 0..top_frame.argc {
                        self.stack.pop();
                    }

                    self.push(ret_val);
                }

                _ => panic!("unknown opcode {:?}", op),
            }
        }
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

        match result
        {
            ExitReason::Exit(value) => value,
            ExitReason::Return(value) => value,
        }
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
        assert!(Op::exit as usize <= 60);
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
        eval_i64("push_i8 3; push_i8 2; push_i8 1; popn 2; exit;", 3);

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
    fn test_call_ret()
    {
        eval_i64("call FN, 0; exit; FN: push_i8 33; ret;", 33);
        eval_i64("push_i8 3; call FN, 1; exit; FN: get_arg 0; push_i8 1; add_u64; ret;", 4);

        // Two arguments and subtract (order of arguments matters)
        eval_i64("push_i8 7; push 5; call FN, 2; exit; FN: get_arg 0; get_arg 1; sub_u64; ret;", 2);

        // Recursive decrement function
        eval_i64("push 10; call DEC, 1; exit; DEC: get_arg 0; dup; jz ZERO; push 1; sub_u64; call DEC, 1; ret; ZERO: ret;", 0);
    }

    #[test]
    fn test_syscalls()
    {
        eval_src(".data; LABEL: .zero 256; .code; push LABEL; push 255; push 0; syscall memset; push 0; exit;");
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
        eval_src(".data; .fill 1000, 0; .code; push 1000; load_u64; exit;");
    }

    #[test]
    #[should_panic]
    fn test_memset_oob()
    {
        eval_src(".data; LABEL: .zero 1; .code; push LABEL; push 255; push 256; syscall memset; push 0; exit;");
    }
}
