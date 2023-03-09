#![allow(dead_code)]
#![allow(unused_variables)]
#![allow(unused_mut)]
#![allow(unused_imports)]

mod vm;
mod sys;
mod asm;

extern crate sdl2;
use std::env;
use std::thread::sleep;
use std::time::Duration;
use std::process::exit;
use crate::vm::{VM, Value, MemBlock, ExitReason};
use crate::asm::{Assembler};

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

fn run_program(vm: &mut VM) -> Value
{
    match vm.call(0, &[])
    {
        ExitReason::Exit(val) => {
            return val;
        }

        // Keep processig events
        ExitReason::Return(val) => {
        }
    }

    loop
    {
        if let ExitReason::Exit(val) = sys::window::process_events(vm) {
            return val;
        }

        let next_cb_time = sys::time::time_until_next_cb(&vm);

        // Sleep until the next callback
        if let Some(delay_ms) = next_cb_time {
            let min_delay = std::cmp::min(delay_ms, 10);
            sleep(Duration::from_millis(min_delay));
        }
        else
        {
            sleep(Duration::from_millis(10));
        }

        // For each callback to run
        for pc in sys::time::get_cbs_to_run(vm)
        {
            match vm.call(pc, &[])
            {
                ExitReason::Exit(val) => {
                    return val;
                }
                ExitReason::Return(val) => {
                }
            }
        }
    }
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
    let result = asm.parse_file(file_name);

    if let Err(error) = &result {
        println!("Error: {}", error);
        exit(-1);
    }

    // Run the program
    if !opts.parse_only {
        let mut vm = result.unwrap();
        let ret_val = run_program(&mut vm);
        exit(ret_val.as_i32());
    }

    exit(0);
}
