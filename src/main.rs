#![allow(dead_code)]
#![allow(unused_variables)]
#![allow(unused_mut)]
#![allow(unused_imports)]

mod vm;
mod asm;
mod display;

use crate::vm::{VM, MemBlock, Op};

fn main() {

    // TODO: read command-line arguments


    let mut code = MemBlock::new();

    code.push_op(Op::nop);

    code.push_op(Op::push_i8);
    code.push_i8(1);

    code.push_op(Op::push_i8);
    code.push_i8(7);

    code.push_op(Op::jmp);
    code.push_i32(1);

    code.push_op(Op::add_i64);

    code.push_op(Op::exit);

    let mut vm = VM::new(code);
    vm.eval();

    if vm.stack_size() > 0
    {
        let ret = vm.pop();
        println!("ret: {:?}", ret);
    }
    else
    {
        println!("stack empty");
    }
}
