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

struct Window<'a>
{
    width: u32,
    height: u32,
    window_id: u32,

    // SDL canvas to draw into
    canvas: sdl2::render::Canvas<sdl2::video::Window>,
    texture_creator: sdl2::render::TextureCreator<sdl2::video::WindowContext>,
    texture: Option<Texture<'a>>,

    // Callbacks for mouse events
    cb_mousemove: u64,
    cb_mousedown: u64,
    cb_mouseup: u64,

    // Callbacks for keyboard events
    cb_keydown: u64,
    cb_keyup: u64,
}

// Note: we're leaving this global to avoid the Window lifetime
// bubbling up everywhere.
// TODO: eventually we will likely want to allow multiple windows
static mut WINDOW: Option<Window> = None;

fn get_window(window_id: u32) -> &'static mut Window<'static>
{
    if window_id != 0 {
        panic!("for now, only one window supported");
    }

    unsafe {
        WINDOW.as_mut().unwrap()
    }
}

pub fn window_create(vm: &mut VM, width: Value, height: Value, title: Value, flags: Value) -> Value
{
    unsafe {
        if WINDOW.is_some() {
            panic!("for now, only one window supported");
        }
    }

    let width: u32 = width.as_usize().try_into().unwrap();
    let height: u32 = height.as_usize().try_into().unwrap();
    let title_str = vm.get_heap_str(title.as_usize()).to_owned();

    let video_subsystem = &mut vm.sys_state.get_window_state().sdl_video;

    let window = video_subsystem.window(&title_str, width, height)
        .hidden()
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
        window_id: 0,
        canvas,
        texture_creator,
        texture: None,
        cb_mousemove: 0,
        cb_mousedown: 0,
        cb_mouseup: 0,
        cb_keydown: 0,
        cb_keyup: 0,
    };

    unsafe {
        WINDOW = Some(window)
    }

    /*
    unsafe {
        let window = WINDOW.as_mut().unwrap();

        // Pixels use the BGRA byte order (0xAA_RR_GG_BB on a little-endian machine)
        window.texture = Some(window.texture_creator.create_texture(
            PixelFormatEnum::BGRA32,
            TextureAccess::Streaming,
            width,
            height
        ).unwrap());
    }
    */

    // TODO: return unique window id
    Value::from(0)
}

pub fn window_draw_frame(vm: &mut VM, window_id: Value, src_addr: Value)
{
    // Get the address to copy pixel data from
    let data_ptr = vm.get_heap_ptr(src_addr.as_usize());

    let window = get_window(window_id.as_u32());

    // If no frame has been drawn yet
    if window.texture.is_none() {
        // Creat the texture to render into
        // Pixels use the BGRA byte order (0xAA_RR_GG_BB on a little-endian machine)
        window.texture = Some(window.texture_creator.create_texture(
            PixelFormatEnum::BGRA32,
            TextureAccess::Streaming,
            window.width,
            window.height
        ).unwrap());

        // We show and raise the window at the moment the first frame is drawn
        // This avoids showing a blank window too early
        window.canvas.window_mut().show();
        window.canvas.window_mut().raise();
    }

    // Update the texture
    let pitch = 4 * window.width as usize;
    let data_len = (4 * window.width * window.height) as usize;
    let pixel_slice = unsafe { std::slice::from_raw_parts(data_ptr, data_len) };
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

pub fn window_on_mousemove(vm: &mut VM, window_id: Value, cb: Value)
{
    let window = get_window(window_id.as_u32());
    window.cb_mousemove = cb.as_u64();
}

pub fn window_on_mousedown(vm: &mut VM, window_id: Value, cb: Value)
{
    let window = get_window(window_id.as_u32());
    window.cb_mousedown = cb.as_u64();
}

pub fn window_on_mouseup(vm: &mut VM, window_id: Value, cb: Value)
{
    let window = get_window(window_id.as_u32());
    window.cb_mouseup = cb.as_u64();
}

pub fn window_on_keydown(vm: &mut VM, window_id: Value, cb: Value)
{
    let window = get_window(window_id.as_u32());
    window.cb_keydown = cb.as_u64();
}

pub fn window_on_keyup(vm: &mut VM, window_id: Value, cb: Value)
{
    let window = get_window(window_id.as_u32());
    window.cb_keyup = cb.as_u64();
}

/// Process SDL events
pub fn process_events(vm: &mut VM) -> bool
{
    let mut event_pump = vm.sys_state.get_sdl_context().event_pump().unwrap();

    // Process all pending events
    // See: https://docs.rs/sdl2/0.30.0/sdl2/event/enum.Event.html
    // TODO: we probably want to process window/input related events in window.rs ?
    for event in event_pump.poll_iter() {
        match event {
            Event::Quit {..} |
            Event::KeyDown { keycode: Some(Keycode::Escape), .. } => {
                return true;
            },

            Event::MouseMotion { window_id, x, y, .. } => {
                window_call_mousemove(vm, window_id, x, y);
            }
            Event::MouseButtonDown { window_id, which, mouse_btn, .. } => {
                window_call_mousedown(vm, window_id, which, mouse_btn);
            }
            Event::MouseButtonUp { window_id, which, mouse_btn, .. } => {
                window_call_mouseup(vm, window_id, which, mouse_btn);
            }

            Event::KeyDown { window_id, keycode: Some(keycode), .. } => {
                window_call_keydown(vm, window_id, keycode);
            },
            Event::KeyUp { window_id, keycode: Some(keycode), .. } => {
                window_call_keyup(vm, window_id, keycode);
            },

            _ => {}
        }
    }

    // TODO: should this return an ExitReason? or Option<ExitReason>?
    return false;
}

// TODO: functions to process window-related events
// TODO: we should return the exit reason?
// this is gonna be awkward if we have audio processing threads/processes and such?
// though I suppose exit would just end those processes

// TODO: this is just for testing
// we should handle window-related events here instead
fn window_call_mousemove(vm: &mut VM, window_id: u32, x: i32, y: i32)
{
    let window = get_window(0);
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
fn window_call_mousedown(vm: &mut VM, window_id: u32, mouse_id: u32, mouse_btn: MouseButton)
{
    let window = get_window(0);
    let cb = window.cb_mousedown;

    if cb == 0 {
        return;
    }

    // TODO: ignore SDL_TOUCH_MOUSEID
    // where is that defined in Rust?
    // or only support mouse id 0?
    //println!("mouse_id={}", mouse_id);

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

fn window_call_mouseup(vm: &mut VM, window_id: u32, mouse_id: u32, mouse_btn: MouseButton)
{
    let window = get_window(0);
    let cb = window.cb_mouseup;

    if cb == 0 {
        return;
    }

    // TODO: ignore SDL_TOUCH_MOUSEID
    // where is that defined in Rust?
    // or only support mouse id 0?
    //println!("mouse_id={}", mouse_id);

    let btn_id = match mouse_btn {
        MouseButton::Left => 0,
        MouseButton::Middle => 1,
        MouseButton::Right => 2,
        MouseButton::X1 => 3,
        MouseButton::X2 => 4,
        // TODO: just don't fire an event for these?
        MouseButton::Unknown => { panic!("wtf"); }
    };

    match vm.call(cb, &[Value::from(window.window_id), Value::from(btn_id)])
    {
        // TODO: we should return the exit reason?
        ExitReason::Exit(val) => {}
        ExitReason::Return(val) => {}
    }
}

fn translate_keycode(sdl_keycode: Keycode) -> Option<u16>
{
    use crate::sys::constants::*;

    match sdl_keycode {
        Keycode::Left => Some(KEY_LEFT),
        Keycode::Right => Some(KEY_RIGHT),
        Keycode::Up => Some(KEY_UP),
        Keycode::Down => Some(KEY_DOWN),
        Keycode::Space => Some(KEY_SPACE),

        _ => None
    }
}

fn window_call_keydown(vm: &mut VM, window_id: u32, keycode: Keycode)
{
    let window = get_window(0);
    let cb = window.cb_keydown;

    let keycode = translate_keycode(keycode);

    if let Some(keycode) = keycode {
        match vm.call(cb, &[Value::from(window.window_id), Value::from(keycode)])
        {
            // TODO: we should return the exit reason?
            ExitReason::Exit(val) => {}
            ExitReason::Return(val) => {}
        }
    }
}

fn window_call_keyup(vm: &mut VM, window_id: u32, keycode: Keycode)
{
    let window = get_window(0);
    let cb = window.cb_keyup;

    let keycode = translate_keycode(keycode);

    if let Some(keycode) = keycode {
        match vm.call(cb, &[Value::from(window.window_id), Value::from(keycode)])
        {
            // TODO: we should return the exit reason?
            ExitReason::Exit(val) => {}
            ExitReason::Return(val) => {}
        }
    }
}