#![allow(dead_code)]
#![allow(unused_variables)]
#![allow(unused_mut)]
#![allow(unused_imports)]

mod vm;
mod syscalls;
mod asm;
mod window;
mod audio;

extern crate sdl2;
use std::env;
use crate::vm::{VM, MemBlock, ExitReason};
use crate::asm::{Assembler};

/// SDL context
pub static mut SDL: Option<sdl2::Sdl> = None;

fn main()
{
    syscalls::init_syscalls();

    // Initialize the SDL context
    unsafe {
        let sdl_context = sdl2::init().unwrap();
        SDL = Some(sdl_context);
    }

    let args: Vec<String> = env::args().collect();
    println!("{:?}", args);

    if args.len() == 2 {
        let asm = Assembler::new();
        let mut vm = asm.parse_file(&args[1]).unwrap();



        let mut event_pump = unsafe {
            SDL.as_mut().unwrap().event_pump().unwrap()
        };

        use sdl2::event::Event;
        use sdl2::keyboard::Keycode;

        let mut i = 0;
        'main_loop: loop {

            let exit_reason = vm.eval();

            match exit_reason
            {
                ExitReason::Exit => {
                    if vm.stack_size() > 0
                    {
                        let ret = vm.pop();
                        println!("return value: {:?}", ret);
                    }

                    return;
                }

                ExitReason::Wait => {
                    // Keep processig events
                }
            }

            for event in event_pump.poll_iter() {
                match event {
                    Event::Quit {..} |
                    Event::KeyDown { keycode: Some(Keycode::Escape), .. } => {
                        break 'main_loop
                    },
                    _ => {}
                }
            }

            //::std::thread::sleep(Duration::new(0, 1_000_000_000u32 / 60));
        }




    }










}
