#![allow(dead_code)]
#![allow(unused_variables)]
#![allow(unused_mut)]
#![allow(unused_imports)]

mod vm;
mod syscalls;
mod asm;
mod window;
mod audio;
mod time;

extern crate sdl2;
use std::env;
use crate::vm::{VM, MemBlock, ExitReason};
use crate::asm::{Assembler};

fn run_program(vm: &mut VM)
{
    use sdl2::event::Event;
    use sdl2::keyboard::Keycode;




    let exit_reason = vm.eval();

    match exit_reason
    {
        ExitReason::Exit => {
            return;
        }

        ExitReason::Wait => {
            // Keep processig events
        }
    }

    let mut event_pump = vm.sys_state.get_sdl_context().event_pump().unwrap();

    let mut i = 0;
    'main_loop: loop {

        // TODO: call back into the VM but only if we have a callback to run

        // Process all pending events
        for event in event_pump.poll_iter() {
            match event {
                Event::Quit {..} |
                Event::KeyDown { keycode: Some(Keycode::Escape), .. } => {
                    break 'main_loop
                },
                _ => {}
            }
        }



        std::thread::sleep( std::time::Duration::from_millis(10) );
    }
}

fn main()
{
    let args: Vec<String> = env::args().collect();
    println!("{:?}", args);

    if args.len() == 2 {
        let asm = Assembler::new();
        let mut vm = asm.parse_file(&args[1]).unwrap();
        run_program(&mut vm);
    }
}
