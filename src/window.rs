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

use crate::vm::{VM, Value};






/*
let mut event_pump = sdl_context.event_pump().unwrap();

let mut event_pump = sdl_context.event_pump().unwrap();
let mut i = 0;
'running: loop {
    i = (i + 1) % 255;
    canvas.set_draw_color(Color::RGB(i, 64, 255 - i));
    canvas.clear();
    for event in event_pump.poll_iter() {
        match event {
            Event::Quit {..} |
            Event::KeyDown { keycode: Some(Keycode::Escape), .. } => {
                break 'running
            },
            _ => {}
        }
    }

    // The rest of the game loop goes here...

    canvas.present();

    //::std::thread::sleep(Duration::new(0, 1_000_000_000u32 / 60));
}
*/







struct Window<'a>
{
    width: u32,
    height: u32,



    // window id
    //window_id


    canvas: sdl2::render::Canvas<sdl2::video::Window>,

    texture_creator: sdl2::render::TextureCreator<sdl2::video::WindowContext>,

    texture: Option<Texture<'a>>,
}





// TODO: eventually we want to allow multiple windows
static mut WINDOW: Option<Window> = None;










pub fn window_create(vm: &mut VM)
{
    let width = 800;
    let height = 600;



    // TODO: move the event loop setup into main
    let sdl_context = sdl2::init().unwrap();







    let video_subsystem = sdl_context.video().unwrap();

    let window = video_subsystem.window("rust-sdl2 demo", width, height)
        .position_centered()
        .build()
        .unwrap();

    let mut canvas = window.into_canvas().build().unwrap();

    canvas.set_draw_color(Color::RGB(0, 255, 255));
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
        ).unwrap() );

    }




}




pub fn window_copy_pixels(vm: &mut VM)
{
    // TODO: get the address


    /*
    fn update<R>(
        &mut self,
        rect: R,
        pixel_data: &[u8],
        pitch: usize
    ) -> Result<(), UpdateTextureError>
    where
        R: Into<Option<Rect>>,
    [−]
    Updates the given texture rectangle with new pixel data.

    pitch is the number of bytes in a row of pixel data, including padding between lines

    If rect is None, the entire texture is updated.
    */




    /*
    pub fn copy<R1, R2>(
        &mut self,
        texture: &Texture<'_>,
        src: R1,
        dst: R2
    ) -> Result<(), String>
    where
        R1: Into<Option<Rect>>,
        R2: Into<Option<Rect>>,
    Copies a portion of the texture to the current rendering target.

    If src is None, the entire texture is copied.
    If dst is None, the texture will be stretched to fill the given rectangle.
    */


    // Update the screen with any rendering performed since the previous call
    //canvas.present();



}
