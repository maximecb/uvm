// Simple display/window device

extern crate sdl2;
use sdl2::pixels::Color;
use sdl2::event::Event;
use sdl2::keyboard::Keycode;
use sdl2::surface::Surface;
use sdl2::render::Texture;
use sdl2::render::TextureAccess;
use sdl2::pixels::PixelFormatEnum;
use std::time::Duration;

use crate::syscalls::{SysState};
use crate::vm::{VM, Value};

struct Window<'a>
{
    width: u32,
    height: u32,

    // TODO: to support multiple windows
    //window_id

    canvas: sdl2::render::Canvas<sdl2::video::Window>,

    texture_creator: sdl2::render::TextureCreator<sdl2::video::WindowContext>,

    texture: Option<Texture<'a>>,
}

/// Mutable state for the window syscalls
pub struct WindowState
{
    // SDL video subsystem
    sdl_video: sdl2::VideoSubsystem,
}

impl SysState
{
    /// Lazily initialize the window state
    fn get_window_state(&mut self) -> &mut WindowState
    {
        if self.window_state.is_none() {

            let video_subsystem = self.get_sdl_context().video().unwrap();

            self.window_state = Some(WindowState {
                sdl_video: video_subsystem
            })
        }

        self.window_state.as_mut().unwrap()
    }
}

// Note: we're leaving this global to avoid the Window lifetime
// bubbling up everywhere.
// TODO: eventually we will likely want to allow multiple windows
static mut WINDOW: Option<Window> = None;

pub fn window_create(vm: &mut VM, width: Value, height: Value)
{
    let width: u32 = width.as_usize().try_into().unwrap();
    let height: u32 = height.as_usize().try_into().unwrap();

    let video_subsystem = &mut vm.sys_state.get_window_state().sdl_video;

    let window = video_subsystem.window("rust-sdl2 demo", width, height)
        .position_centered()
        .build()
        .unwrap();

    let mut canvas = window.into_canvas().build().unwrap();

    canvas.set_draw_color(Color::RGB(0, 0, 0));
    canvas.clear();
    canvas.present();

    let texture_creator = canvas.texture_creator();

    let window = Window {
        width,
        height,
        canvas,
        texture_creator,
        texture: None,
    };

    unsafe {
        WINDOW = Some(window)
    }

    unsafe {
        let window = WINDOW.as_mut().unwrap();

        window.texture = Some( window.texture_creator.create_texture(
            PixelFormatEnum::RGB24,
            TextureAccess::Streaming,
            width,
            height
        ).unwrap());
    }
}

pub fn window_show(vm: &mut VM)
{
    //println!("show the window");

    unsafe {
        let mut window = WINDOW.as_mut().unwrap();
        window.canvas.window_mut().show();

        let (width, height) = window.canvas.window().size();
        println!("width={}, height={}", width, height);
    }
}

pub fn window_copy_pixels(vm: &mut VM, src_addr: Value)
{
    // Get the address to copy pixel data from
    let data_ptr = vm.get_heap_ptr(src_addr.as_usize());

    unsafe {
        let mut window = WINDOW.as_mut().unwrap();

        // Update the texture
        let pitch = 3 * window.width as usize;
        let data_len = (3 * window.width * window.height) as usize;
        let pixel_slice = std::slice::from_raw_parts(data_ptr, data_len);
        window.texture.as_mut().unwrap().update(None, pixel_slice, pitch).unwrap();

        // Copy the texture into the canvas
        window.canvas.copy(
            &window.texture.as_ref().unwrap(),
            None,
            None
        ).unwrap();

        // Update the screen with any rendering performed since the previous call
        window.canvas.present();
    }
}
