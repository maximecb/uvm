#![allow(dead_code)]

enum Op
{
    Nop,
    Add,
    Sub,
}

struct Value(u64);

struct VM
{
    heap: Vec<u8>,


    exec_mem: Vec<u8>,


    stack: Vec<Value>,



    // Points at a byte in the executable memory
    pc: usize,




}
