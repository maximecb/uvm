#![allow(dead_code)]

/// Instruction opcodes
#[allow(non_camel_case_types)]
enum Op
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
    //sub,



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

struct Value(u64);

struct VM
{
    heap: Vec<u8>,


    exec_mem: Vec<u8>,


    // Value stack
    stack: Vec<Value>,

    // Call stack?


    // Points at a byte in the executable memory
    pc: usize,




}

impl VM
{
    fn new() -> Self
    {
        todo!();
    }

    fn eval()
    {

    }
}
