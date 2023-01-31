#![allow(unused_imports)]
#![allow(dead_code)]
#![allow(unused_variables)]
#![allow(unused_parens)]
#![allow(unused_mut)]

mod parser;
mod ast;
mod symbols;
mod types;
mod codegen;
mod exec_tests;

use std::env;
use parser::*;
use ast::*;
use symbols::*;
use types::*;
use codegen::*;

fn main()
{
    let args: Vec<String> = env::args().collect();
    println!("{:?}", args);

    // If an input file was specified
    if args.len() == 2 {
        let mut unit = parse_file(&args[1]).unwrap();

        unit.resolve_syms().unwrap();
        unit.check_types().unwrap();
        let out = unit.gen_code().unwrap();

        std::fs::write("out.asm", out).unwrap();
    }
}
