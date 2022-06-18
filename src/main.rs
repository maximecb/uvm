#![allow(dead_code)]
#![allow(unused_variables)]
#![allow(unused_mut)]

mod vm;
mod display;

use crate::vm::{VM, MemBlock, Op};

fn main() {

    // TODO: read command-line arguments


    let mut code = MemBlock::new();

    code.push_op(Op::nop);

    code.push_op(Op::push_i8);
    code.push_i8(7);

    code.push_op(Op::exit);

    let mut vm = VM::new(code);

    vm.eval();
}
