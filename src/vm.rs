use std::mem::transmute;

/// Instruction opcodes
/// Note: commonly used upcodes should be in the [0, 127] range (one byte)
#[allow(non_camel_case_types)]
#[repr(u8)]
pub enum Op
{
    // Halt execution and produce an error
    // This is opcode zero so that jumping to uninitialized memory halts
    halt = 0,

    // No-op (useful for code patching)
    nop,

    // push_i8 <i8_imm> (sign-extended)
    push_i8,

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

    pub fn write_u8(&mut self, pos: usize, val: u8)
    {
        self.data[pos] = val;
    }

    pub fn write_i32(&mut self, pos: usize, val: i32)
    {
        unsafe {
            let buf_ptr = self.data.as_mut_ptr();
            let val_ptr = transmute::<*mut u8 , *mut i32>(buf_ptr.add(pos));
            *val_ptr = val;
        }
    }

    pub fn read_op(&self, pos: usize) -> Op
    {
        unsafe {
            transmute::<u8 , Op>(self.data[pos])
        }
    }

    pub fn read_u8(&self, pos: usize) -> u8
    {
        self.data[pos]
    }

    pub fn read_i8(&self, pos: usize) -> i8
    {
        self.data[pos] as i8
    }

    pub fn read_i32(&self, pos: usize) -> i32
    {
        unsafe {
            let buf_ptr = self.data.as_ptr();
            let val_ptr = transmute::<*const u8 , *const i32>(buf_ptr.add(pos));
            *val_ptr
        }
    }
}

pub struct VM
{
    heap: MemBlock,

    code: MemBlock,

    // Value stack
    stack: Vec<Value>,

    // TODO
    // Call stack? Do we need one

    // Points at a byte in the executable memory
    pc: usize,
}

impl VM
{
    pub fn new(code: MemBlock) -> Self
    {
        Self {
            code,
            heap: MemBlock::new(),
            stack: Vec::default(),
            pc: 0,
        }
    }

    pub fn stack_size(&self) -> usize
    {
        self.stack.len()
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

            let op = self.code.read_op(self.pc);
            self.pc += 1;

            match op
            {
                Op::halt => panic!("execution error, encountered halt opcode"),

                Op::nop => continue,

                Op::exit => break,

                Op::push_i8 => {
                    let val = self.code.read_i8(self.pc);
                    self.pc += 1;
                    self.stack.push(Value::from_i8(val));
                }

                Op::add_i64 => {
                    let v0 = self.pop();
                    let v1 = self.pop();
                    self.stack.push(Value::from_i64(
                        v0.as_i64() + v1.as_i64()
                    ));
                }

                Op::jmp => {
                    let offset = self.code.read_i32(self.pc) as isize;
                    self.pc += 4;
                    self.pc = ((self.pc as isize) + offset) as usize;
                }

                Op::jnz => {
                    let offset = self.code.read_i32(self.pc) as isize;
                    self.pc += 4;

                    let v0 = self.pop();

                    if v0.as_i64() != 0 {
                        self.pc = ((self.pc as isize) + offset) as usize;
                    }
                }

                _ => panic!("unknown opcode"),
            }
        }
    }
}
