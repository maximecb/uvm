#![allow(dead_code)]
#![allow(unused_variables)]
#![allow(unused_mut)]
#![allow(unused_parens)]
#![allow(unused_imports)]

mod window;
mod audio;
mod net;
mod time;
mod constants;
mod host;
mod vm;
mod asm;
mod program;
mod utils;

extern crate sdl2;
extern crate libc;
use std::env;
use std::thread::sleep;
use std::time::Duration;
use std::process::exit;
use std::sync::{Arc, Mutex};
use crate::vm::{VM, Value};
use crate::asm::{Assembler};
use crate::utils::{thousands_sep};

/// Command-line options
#[derive(Debug, Clone)]
struct Options
{
    // Only parse/validate the input, but don't run it
    parse_only: bool,

    rest: Vec<String>,
}

// TODO: parse permissions
// --allow <permissions>
// --deny <permissions>
// --allow-all
fn parse_args(args: Vec<String>) -> Options
{
    let mut opts = Options {
        parse_only: false,
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
            "--parse-only" => {
                opts.parse_only = true;
            }

            _ => panic!("unknown option {}", arg)
        }
    }

    opts
}

fn main()
{
    let opts = parse_args(env::args().collect());
    //println!("{:?}", opts);

    if opts.rest.len() != 1 {
        panic!("must specify exactly one input file to run");
    }

    let file_name = &opts.rest[0];

    // Parse/compile the program
    let asm = Assembler::new();
    let program = asm.parse_file(file_name);

    if let Err(error) = program {
        println!("Error: {}", error);
        exit(-1);
    }

    // Run the program
    if opts.parse_only {
        exit(0);
    }

    let program = program.unwrap();
    let mut vm = VM::new(program);
    let ret_val = VM::call(&mut vm, 0, &[]);

    exit(ret_val.as_i32());
}
