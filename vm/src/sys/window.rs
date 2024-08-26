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

use crate::sys::{SysState, get_sdl_context};
use crate::vm::{VM, Value, ExitReason};

/// SDL video subsystem
/// This is a global variable because it doesn't implement
/// the Send trait, and so can't be referenced from another thread
static mut SDL_VIDEO: Option<sdl2::VideoSubsystem> = None;

/// Lazily initialize the SDL video subsystem
fn get_video_subsystem() -> &'static mut sdl2::VideoSubsystem
{
    unsafe
    {
        let sdl = get_sdl_context();

        if SDL_VIDEO.is_none() {
            SDL_VIDEO = Some(sdl.video().unwrap());
        }

        SDL_VIDEO.as_mut().unwrap()
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

    let video_subsystem = get_video_subsystem();

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
    };

    unsafe {
        WINDOW = Some(window)
    }

    // TODO: return unique window id
    Value::from(0)
}

pub fn window_draw_frame(vm: &mut VM, window_id: Value, src_addr: Value)
{
    let window = get_window(window_id.as_u32());

    // Get the address to copy pixel data from
    let data_len = (4 * window.width * window.height) as usize;
    let data_ptr = vm.get_heap_ptr(src_addr.as_usize(), data_len);

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





/*
/// Process SDL events
pub fn process_events(vm: &mut VM) -> ExitReason
{
    let mut event_pump = get_sdl_context().event_pump().unwrap();

    // Process all pending events
    // See: https://docs.rs/sdl2/0.30.0/sdl2/event/enum.Event.html
    // TODO: we probably want to process window/input related events in window.rs ?
    for event in event_pump.poll_iter() {
        match event {
            Event::Quit { .. } => {
                return ExitReason::Exit(Value::from(0));
            }

            Event::MouseMotion { window_id, x, y, .. } => {
                if let ExitReason::Exit(val) = window_call_mousemove(vm, window_id, x, y) {
                    return ExitReason::Exit(val);
                }
            }

            Event::MouseButtonDown { window_id, which, mouse_btn, x, y, .. } => {
                if let ExitReason::Exit(val) = window_call_mousedown(vm, window_id, mouse_btn, x, y) {
                    return ExitReason::Exit(val);
                }
            }

            Event::MouseButtonUp { window_id, which, mouse_btn, x, y, .. } => {
                if let ExitReason::Exit(val) = window_call_mouseup(vm, window_id, mouse_btn, x, y) {
                    return ExitReason::Exit(val);
                }
            }

            Event::KeyDown { window_id, keycode: Some(keycode), .. } => {
                if let ExitReason::Exit(val) = window_call_keydown(vm, window_id, keycode) {
                    return ExitReason::Exit(val);
                }
            }

            Event::KeyUp { window_id, keycode: Some(keycode), .. } => {
                if let ExitReason::Exit(val) = window_call_keyup(vm, window_id, keycode) {
                    return ExitReason::Exit(val);
                }
            }

            Event::TextInput { window_id, text, .. } => {
                // For each UTF-8 byte of input
                for ch in text.bytes() {
                    if let ExitReason::Exit(val) = window_call_textinput(vm, window_id, ch) {
                        return ExitReason::Exit(val);
                    }
                }
            }

            _ => {}
        }
    }

    return ExitReason::default();
}
*/





/*
fn translate_keycode(sdl_keycode: Keycode) -> Option<u16>
{
    use crate::sys::constants::*;

    // https://docs.rs/sdl2/0.30.0/sdl2/keyboard/enum.Keycode.html
    match sdl_keycode {
        Keycode::A => Some(KEY_A),
        Keycode::B => Some(KEY_B),
        Keycode::C => Some(KEY_C),
        Keycode::D => Some(KEY_D),
        Keycode::E => Some(KEY_E),
        Keycode::F => Some(KEY_F),
        Keycode::G => Some(KEY_G),
        Keycode::H => Some(KEY_H),
        Keycode::I => Some(KEY_I),
        Keycode::J => Some(KEY_J),
        Keycode::K => Some(KEY_K),
        Keycode::L => Some(KEY_L),
        Keycode::M => Some(KEY_M),
        Keycode::N => Some(KEY_N),
        Keycode::O => Some(KEY_O),
        Keycode::P => Some(KEY_P),
        Keycode::Q => Some(KEY_Q),
        Keycode::R => Some(KEY_R),
        Keycode::S => Some(KEY_S),
        Keycode::T => Some(KEY_T),
        Keycode::U => Some(KEY_U),
        Keycode::V => Some(KEY_V),
        Keycode::W => Some(KEY_W),
        Keycode::X => Some(KEY_X),
        Keycode::Y => Some(KEY_Y),
        Keycode::Z => Some(KEY_Z),

        Keycode::Num0 => Some(KEY_NUM0),
        Keycode::Num1 => Some(KEY_NUM1),
        Keycode::Num2 => Some(KEY_NUM2),
        Keycode::Num3 => Some(KEY_NUM3),
        Keycode::Num4 => Some(KEY_NUM4),
        Keycode::Num5 => Some(KEY_NUM5),
        Keycode::Num6 => Some(KEY_NUM6),
        Keycode::Num7 => Some(KEY_NUM7),
        Keycode::Num8 => Some(KEY_NUM8),
        Keycode::Num9 => Some(KEY_NUM9),

        Keycode::Comma => Some(KEY_COMMA),
        Keycode::Period => Some(KEY_PERIOD),
        Keycode::Slash => Some(KEY_SLASH),
        Keycode::Colon => Some(KEY_COLON),
        Keycode::Semicolon => Some(KEY_SEMICOLON),
        Keycode::Equals => Some(KEY_EQUALS),
        Keycode::Question => Some(KEY_QUESTION),

        Keycode::Escape => Some(KEY_ESCAPE),
        Keycode::Backspace => Some(KEY_BACKSPACE),
        Keycode::Left => Some(KEY_LEFT),
        Keycode::Right => Some(KEY_RIGHT),
        Keycode::Up => Some(KEY_UP),
        Keycode::Down => Some(KEY_DOWN),
        Keycode::Space => Some(KEY_SPACE),
        Keycode::Return => Some(KEY_RETURN),
        Keycode::LShift => Some(KEY_SHIFT),
        Keycode::RShift => Some(KEY_SHIFT),
        Keycode::Tab => Some(KEY_TAB),

        _ => None
    }
}
*/
