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
use crate::vm::{VM, Value, MemBlock, ExitReason};
use crate::asm::{Assembler};

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

    let mut i = 0;
    'main_loop: loop
    {
        let quit = sys::window::process_events(vm);
        if quit {
            break 'main_loop;
        }

        let next_cb_time = sys::time::time_until_next_cb(&vm);

        // Sleep until the next callback
        if let Some(delay_ms) = next_cb_time {
            sleep(Duration::from_millis(delay_ms));
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

    Value::from(0 as u32)
}

fn main()
{
    let args: Vec<String> = env::args().collect();
    //println!("{:?}", args);

    // TODO: command-line argument parsing
    // --allow <permissions>
    // --deny <permissions>
    // --allow-all

    if args.len() == 2 {
        let asm = Assembler::new();
        let mut vm = asm.parse_file(&args[1]).unwrap();
        let ret_val = run_program(&mut vm);
        std::process::exit(ret_val.as_i32());
    }

    std::process::exit(0);
}
