
/// Instruction opcodes
#[allow(non_camel_case_types)]
#[repr(u8)]
pub enum Op
{
    // Halt execution and produce an error
    halt = 0,

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

    # End execution with an integer return code
    exit


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

pub struct MemBlock
{
    data: Vec<u8>
}

impl MemBlock
{
    pub fn new() -> Self
    {
        todo!();
    }

    pub fn append_u8(&self, val: u8)
    {

    }



    // NOTE: do we want to write at some position?
    // write vs append
    pub fn write_u8(&self, val: u8)
    {
    }

    pub fn read_u8()
    {
    }





}

pub struct VM
{
    heap: MemBlock,


    code: MemBlock,


    // Value stack
    stack: Vec<Value>,



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

    pub fn eval()
    {

    }
}
