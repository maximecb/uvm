#![allow(dead_code)]
#![allow(unused_variables)]

mod vm;
mod display;

use crate::vm::{VM, MemBlock, Op};

fn main() {
    println!("Hello, world!");

    let mut code = MemBlock::new();

    code.push_op(Op::halt);

    let vm = VM::new(code);





}
