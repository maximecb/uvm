// Simple display/window device

extern crate sdl2;
use sdl2::pixels::Color;
use sdl2::event::Event;
use sdl2::keyboard::Keycode;
use sdl2::mouse::MouseButton;
use sdl2::surface::Surface;
use sdl2::render::Texture;
use sdl2::render::TextureAccess;
use sdl2::pixels::PixelFormatEnum;

use std::time::Duration;

use crate::sys::{SysState};
use crate::vm::{VM, Value, ExitReason};

struct Window<'a>
{
    width: u32,
    height: u32,

    // TODO: we should support multiple windows
    //window_id

    // SDL canvas to draw into
    canvas: sdl2::render::Canvas<sdl2::video::Window>,
    texture_creator: sdl2::render::TextureCreator<sdl2::video::WindowContext>,
    texture: Option<Texture<'a>>,

    // Callbacks for mouse events
    cb_mousemove: u64,
    cb_mousedown: u64,
    cb_mouseup: u64,
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

pub fn window_create(vm: &mut VM, width: Value, height: Value, title: Value, flags: Value) -> Value
{
    let width: u32 = width.as_usize().try_into().unwrap();
    let height: u32 = height.as_usize().try_into().unwrap();
    let title_str = vm.get_heap_str(title.as_usize()).to_owned();

    let video_subsystem = &mut vm.sys_state.get_window_state().sdl_video;

    let window = video_subsystem.window(&title_str, width, height)
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
        cb_mousemove: 0,
        cb_mousedown: 0,
        cb_mouseup: 0,
    };

    unsafe {
        WINDOW = Some(window)
    }

    unsafe {
        let window = WINDOW.as_mut().unwrap();

        window.texture = Some(window.texture_creator.create_texture(
            PixelFormatEnum::RGB24,
            TextureAccess::Streaming,
            width,
            height
        ).unwrap());
    }

    // TODO: return window id
    Value::from(0)
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

pub fn window_on_mousemove(vm: &mut VM, window_id: Value, cb: Value)
{
    let cb = cb.as_u64();

    unsafe {
        let mut window = WINDOW.as_mut().unwrap();
        window.cb_mousemove = cb;
    }
}

pub fn window_on_mousedown(vm: &mut VM, window_id: Value, cb: Value)
{
    let cb = cb.as_u64();

    unsafe {
        let mut window = WINDOW.as_mut().unwrap();
        window.cb_mousedown = cb;
    }
}

pub fn window_on_mouseup(vm: &mut VM, window_id: Value, cb: Value)
{
    let cb = cb.as_u64();

    unsafe {
        let mut window = WINDOW.as_mut().unwrap();
        window.cb_mouseup = cb;
    }
}

// TODO: functions to process window-related events
// TODO: we should return the exit reason?
// this is gonna be awkward if we have audio processing threads/processes and such?
// though I suppose exit would just end those processes

// TODO: this is just for testing
// we should handle window-related events here instead
pub fn window_call_mousemove(vm: &mut VM, window_id: u32, x: i32, y: i32)
{
    unsafe {
        let mut window = WINDOW.as_mut().unwrap();
        let cb = window.cb_mousemove;

        if cb == 0 {
            return;
        }

        // TODO: pass window id
        match vm.call(cb, &[Value::from(0), Value::from(x), Value::from(y)])
        {
            // TODO: we should return the exit reason?
            ExitReason::Exit(val) => {}
            ExitReason::Return(val) => {}
        }
    }
}

/*
MouseButtonDown {
    timestamp: u32,
    window_id: u32,
    which: u32, => this is a mouse id
    mouse_btn: MouseButton,
    x: i32,
    y: i32,
},
*/
pub fn window_call_mousedown(vm: &mut VM, window_id: u32, mouse_id: u32, mouse_btn: MouseButton)
{
    unsafe {
        let mut window = WINDOW.as_mut().unwrap();
        let cb = window.cb_mousedown;

        if cb == 0 {
            return;
        }

        // TODO: ignore
        //SDL_TOUCH_MOUSEID
        // where is that defined in Rust?
        // or just support mouse id 0?
        println!("mouse_id={}", mouse_id);

        let btn_id = match mouse_btn {
            MouseButton::Left => 0,
            MouseButton::Middle => 1,
            MouseButton::Right => 2,
            MouseButton::X1 => 3,
            MouseButton::X2 => 4,
            // TODO: just don't fire an event for these?
            MouseButton::Unknown => { panic!("wtf"); }
        };

        // TODO: pass window id
        match vm.call(cb, &[Value::from(0), Value::from(btn_id)])
        {
            // TODO: we should return the exit reason?
            ExitReason::Exit(val) => {}
            ExitReason::Return(val) => {}
        }
    }
}

pub fn window_call_mouseup(vm: &mut VM, window_id: u32, mouse_id: u32, mouse_btn: MouseButton)
{
    unsafe {
        let mut window = WINDOW.as_mut().unwrap();
        let cb = window.cb_mouseup;

        if cb == 0 {
            return;
        }

        // TODO: ignore
        //SDL_TOUCH_MOUSEID
        // where is that defined in Rust?
        // or just support mouse id 0?
        println!("mouse_id={}", mouse_id);

        let btn_id = match mouse_btn {
            MouseButton::Left => 0,
            MouseButton::Middle => 1,
            MouseButton::Right => 2,
            MouseButton::X1 => 3,
            MouseButton::X2 => 4,
            // TODO: just don't fire an event for these?
            MouseButton::Unknown => { panic!("wtf"); }
        };

        // TODO: pass window id
        match vm.call(cb, &[Value::from(0), Value::from(btn_id)])
        {
            // TODO: we should return the exit reason?
            ExitReason::Exit(val) => {}
            ExitReason::Return(val) => {}
        }
    }
}
