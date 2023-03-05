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

#[derive(Debug, Clone)]
struct Options
{
    // Print the preprocessor output
    print_cpp_out: bool,

    // Output file
    out_file: String,

    rest: Vec<String>,
}

fn parse_args(args: Vec<String>) -> Options
{
    let mut opts = Options {
        print_cpp_out: false,
        out_file: "out.asm".to_string(),
        rest: Vec::default(),
    };

    // Start parsing at argument 1 because 0 is the current program name
    let mut idx = 1;

    while idx < args.len() {
        let arg = &args[idx];
        //println!("{}", arg);

        // If this is the start of the rest arguments
        if !arg.starts_with("-") {
            opts.rest = args[idx..].to_vec();
            break;
        }

        // Move to the next argument
        idx += 1;

        // Try to match this argument as an option
        match arg.as_str() {
            "-E" => {
                opts.print_cpp_out = true;
            }

            "-o" => {
                opts.out_file = args[idx].clone();
                idx += 1;
            }

            _ => panic!("unknown option {}", arg)
        }
    }

    opts
}

fn compile_file(file_name: &str, opts: &Options) -> Result<(), ParseError>
{
    let mut input = Input::from_file(file_name);

    let output = process_input(&mut input)?;

    if opts.print_cpp_out {
        println!("{}", output);
    }

    let mut input = Input::new(&output, file_name);
    let mut unit = parse_unit(&mut input)?;

    unit.resolve_syms()?;
    unit.check_types()?;
    let out = unit.gen_code()?;

    std::fs::write(&opts.out_file, out).unwrap();

    Ok(())
}

fn main()
{
    let opts = parse_args(env::args().collect());
    //println!("{:?}", opts);

    if opts.rest.len() != 1 {
        panic!("Must specify exactly one input source file to compile.");
    }

    let file_name = &opts.rest[0];
    let result = compile_file(file_name, &opts);

    if let Err(error) = result {
        if error.line_no != 0 {
            println!("Error @{}:{}: {}", error.line_no, error.col_no, error.msg);
        } else
        {
            println!("Error: {}", error.msg);
        }

        std::process::exit(-1);
    }
}
