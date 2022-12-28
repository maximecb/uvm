#![allow(unused_imports)]
#![allow(dead_code)]
#![allow(unused_variables)]
#![allow(unused_parens)]
#![allow(unused_mut)]

use std::env;

mod parser;
use parser::*;

mod ast;
use ast::*;

mod symbols;
mod types;

fn main()
{
    let args: Vec<String> = env::args().collect();
    println!("{:?}", args);

    // If an input file was specified
    if args.len() == 2 {
        //let unit_fn = parse_file(&mut vm, &args[1]).unwrap();



    }
}
