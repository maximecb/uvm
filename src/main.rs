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

    match vm.eval()
    {
        ExitReason::Exit => {
            dbg!(vm.stack_size());
            return
        },

        // Keep processig events
        ExitReason::Wait => {}
    }

    let mut event_pump = vm.sys_state.get_sdl_context().event_pump().unwrap();

    let mut i = 0;
    'main_loop: loop
    {
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

        let next_cb_time = time::time_until_next_cb(&vm);

        if let Some(delay_ms) = next_cb_time {
            std::thread::sleep(std::time::Duration::from_millis(delay_ms));
        }
        else
        {
            std::thread::sleep(std::time::Duration::from_millis(10));
        }

        // For each callback to run
        for pc in time::get_cbs_to_run(vm)
        {
            vm.set_pc(pc);

            match vm.eval()
            {
                ExitReason::Exit => { return },
                ExitReason::Wait => {}
            }

            dbg!(vm.stack_size());
        }
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
