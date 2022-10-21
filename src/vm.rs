use std::mem::{transmute, size_of};
use crate::syscalls::{SyscallFn, get_syscall};

/// Instruction opcodes
/// Note: commonly used upcodes should be in the [0, 127] range (one byte)
///       less frequently used opcodes can take multiple bytes if necessary.
#[allow(non_camel_case_types)]
#[derive(PartialEq, Copy, Clone, Debug)]
#[repr(u8)]
pub enum Op
{
    // Halt execution and produce an error
    // This is opcode zero so that jumping to uninitialized memory halts
    halt = 0,

    // No-op (useful for code patching or patch points)
    nop,

    // push_i8 <i8_imm> (sign-extended)
    push_i8,

    // push_u32 <u32_imm>
    push_u32,

    // push_u64 <u64_imm>
    push_u64,

    // Stack manipulation
    swap,
    pop,
    dup,

    /*
    /// Load from heap at fixed address
    /// This is used for reading global variables
    /// The address is multiplied by the data size (x 4 or x8)
    /// If we save 24 bits for the offset, then that gives us quite a lot
    load_static <address>

    load
    store
    memcpy
    */

    /*
    // Bitwise operations
    and
    or
    not
    lshift
    */

    // Integer arithmetic
    add_i64,
    sub_i64,
    mul_i64,

    /*
    # Test flag bits (logical and) with a constant
    # This can be used for tag bit tests
    test_u8 <u8_flags>

    # Comparisons
    eq_i64
    lt_i64
    gt_i64
    ge_i64
    …
    */

    // Jump to pc offset
    jmp,

    // Jump to pc offset if stack top is zero
    jz,

    // Jump to pc offset if stack top is not zero
    jnz,

    // Jump to pc offset if top values equal
    jne,

    // Call and return using the call stack
    //call
    //ret

    // Call into a blocking host function
    // For example, to set up a device or to allocate more memory
    // syscall <device_id:u16> <method_id:u16>
    syscall,

    /*
    // Wait for a callback from the host or a device (go into a waiting state)
    // Ideally the stack should be fully unwound when this is called,
    // we can relax this assumption later
    wait

    # Suspend execution, release devices, save image
    # Ideally the stack should be unwound when this is called,
    # we can relax this assumption later
    suspend
    */

    // End execution normally
    exit,
}

#[derive(Debug)]
pub struct Value(u64);

impl Value
{
    fn from_i8(val: i8) -> Self
    {
        Value((val as i64) as u64)
    }

    fn from_i64(val: i64) -> Self
    {
        Value(val as u64)
    }

    fn from_u32(val: u32) -> Self
    {
        Value(val as u64)
    }

    fn from_u64(val: u64) -> Self
    {
        Value(val)
    }

    fn as_i64(&self) -> i64 {
        let Value(val) = *self;
        val as i64
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

pub struct VM
{
    /// Table of system calls the program can refer to
    syscalls: Vec<SyscallFn>,

    heap: MemBlock,

    code: MemBlock,

    // Value stack
    stack: Vec<Value>,


    // TODO
    // Call stack? Do we need one
    // Would prefer not to expose this so we can swap stacks

    // Points at a byte in the executable memory
    pc: usize,
}

impl VM
{
    pub fn new(code: MemBlock, heap: MemBlock, syscalls: Vec<String>) -> Self
    {
        let mut syscall_fns = Vec::new();

        for syscall_name in syscalls {
            syscall_fns.push(get_syscall(&syscall_name));
        }

        Self {
            syscalls: syscall_fns,
            code,
            heap,
            stack: Vec::default(),
            pc: 0,
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

    pub fn pop(&mut self) -> Value
    {
        self.stack.pop().unwrap()
    }

    pub fn eval(&mut self)
    {
        loop
        {
            if self.pc >= self.code.len() {
                panic!("pc out of bounds")
            }

            let op = self.code.read_pc::<Op>(&mut self.pc);

            match op
            {
                Op::halt => panic!("execution error, encountered halt opcode"),

                Op::nop => continue,

                Op::exit => break,

                Op::pop => {
                    self.pop();
                }

                Op::push_i8 => {
                    let val = self.code.read_pc::<i8>(&mut self.pc);
                    self.stack.push(Value::from_i8(val));
                }

                Op::push_u32 => {
                    let val = self.code.read_pc::<u32>(&mut self.pc);
                    self.push(Value::from_u32(val));
                }

                Op::push_u64 => {
                    let val = self.code.read_pc::<u64>(&mut self.pc);
                    self.push(Value::from_u64(val));
                }

                Op::add_i64 => {
                    let v0 = self.pop();
                    let v1 = self.pop();
                    self.stack.push(Value::from_i64(
                        v0.as_i64() + v1.as_i64()
                    ));
                }

                Op::jmp => {
                    let offset = self.code.read_pc::<i32>(&mut self.pc) as isize;
                    self.pc = ((self.pc as isize) + offset) as usize;
                }

                Op::jnz => {
                    let offset = self.code.read_pc::<i32>(&mut self.pc) as isize;
                    let v0 = self.pop();

                    if v0.as_i64() != 0 {
                        self.pc = ((self.pc as isize) + offset) as usize;
                    }
                }

                Op::jne => {
                    let offset = self.code.read_pc::<i32>(&mut self.pc) as isize;
                    let v0 = self.pop();
                    let v1 = self.pop();

                    if v0.as_i64() != v1.as_i64() {
                        self.pc = ((self.pc as isize) + offset) as usize;
                    }
                }

                Op::syscall => {
                    let table_idx = self.code.read_pc::<u16>(&mut self.pc) as usize;

                    assert!(table_idx < self.syscalls.len());
                    let syscall_fn = self.syscalls[table_idx];

                    syscall_fn(self);
                }

                _ => panic!("unknown opcode"),
            }
        }
    }
}
