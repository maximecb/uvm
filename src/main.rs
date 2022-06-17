#![allow(dead_code)]
#![allow(unused_variables)]
#![allow(unused_mut)]

mod vm;
mod display;

use crate::vm::{VM, MemBlock, Op};

fn main() {

    // TODO: read command-line arguments


    let mut code = MemBlock::new();

    code.push_op(Op::halt);

    let mut vm = VM::new(code);

    vm.eval();




}
