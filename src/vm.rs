/// Instruction opcodes
#[allow(non_camel_case_types)]
#[repr(u8)]
pub enum Op
{
    // Halt execution and produce an error
    halt = 0,

    // End execution normally
    exit,

    // No-op
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
    # Load from heap at address x 4 or x8
    # If we save 24 bits for the offset, then that gives us quite a lot
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
    //sub_i64,

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

    if_true
    if_false
    jump


    # Call and return using the call stack
    call
    ret

    # Call into a blocking host function
    # For example, to set up a device or to allocate more memory
    syscall

    # Wait for a callback from the host or a device (go into a waiting state)
    # Ideally the stack should be fully unwound when this is called,
    # we can relax this assumption later
    wait

    # Suspend execution, release devices, save image
    # Ideally the stack should be unwound when this is called,
    # we can relax this assumption later
    suspend
    */
}

pub struct Value(u64);

impl Value
{
    fn from_i8(val: i8) -> Self
    {
        Value((val as i64) as u64)
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

    pub fn push_i8(&mut self, val: i8)
    {
        self.data.push(val as u8);
    }

    pub fn write_u8(&mut self, pos: usize, val: u8)
    {
        self.data[pos] = val;
    }

    pub fn read_op(&self, pos: usize) -> Op
    {
        unsafe {
            std::mem::transmute::<u8 , Op>(self.data[pos])
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

    pub fn eval(&mut self)
    {
        loop
        {
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

                _ => panic!("unknown opcode"),
            }
        }
    }
}
