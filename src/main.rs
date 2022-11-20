#![allow(dead_code)]
#![allow(unused_variables)]
#![allow(unused_mut)]
#![allow(unused_imports)]

mod vm;
mod syscalls;
mod asm;
mod window;
mod audio;

extern crate sdl2;
use std::env;
use crate::vm::{VM, MemBlock, Op};
use crate::asm::{Assembler};



/// SDL context
static mut SDL: Option<sdl2::Sdl> = None;







fn main()
{
    syscalls::init_syscalls();











    let args: Vec<String> = env::args().collect();
    println!("{:?}", args);

    if args.len() == 2 {
        let asm = Assembler::new();
        let mut vm = asm.parse_file(&args[1]).unwrap();

        vm.eval();

        if vm.stack_size() > 0
        {
            let ret = vm.pop();
            println!("return value: {:?}", ret);
        }
        else
        {
            println!("vm stack empty");
        }

        return;
    }




}
