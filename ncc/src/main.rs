#![allow(unused_imports)]
#![allow(dead_code)]
#![allow(unused_variables)]
#![allow(unused_parens)]
#![allow(unused_mut)]

mod parsing;
mod cpp;
mod parser;
mod ast;
mod symbols;
mod types;
mod codegen;
mod exec_tests;

use std::env;
use parsing::*;
use cpp::*;
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
        let file_name = &args[1];

        let mut input = Input::from_file(file_name);
        let output = process_input(&mut input).unwrap();

        println!("{}", output);

        let mut input = Input::new(&output, file_name);
        let mut unit = parse_unit(&mut input).unwrap();
        unit.resolve_syms().unwrap();
        unit.check_types().unwrap();
        let out = unit.gen_code().unwrap();

        std::fs::write("out.asm", out).unwrap();
    }
}
