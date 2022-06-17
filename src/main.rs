#![allow(dead_code)]
#![allow(unused_variables)]

mod vm;
mod display;

use crate::vm::{VM, MemBlock};

fn main() {
    println!("Hello, world!");

    let vm = VM::new(MemBlock::new());





}
