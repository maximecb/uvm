use sdl2::audio::{AudioCallback, AudioSpecDesired, AudioDevice};
use std::sync::{Arc, Weak, Mutex};
use std::collections::HashMap;
use std::cell::RefCell;
use crate::vm::{Value, VM, Thread};
use crate::host::{get_sdl_context};
use crate::constants::*;

struct AudioCB
{
    // Number of audio output channels
    num_channels: usize,

    // VM thread in which to execute the audio callback
    thread: Thread,

    // Callback function pointer
    cb: u64,
}

impl AudioCallback for AudioCB
{
    // Signed 16-bit samples
    type Channel = i16;

    fn callback(&mut self, out: &mut [i16])
    {
        out.fill(0);

        let output_len = out.len();
        assert!(output_len % self.num_channels == 0);
        let samples_per_chan = output_len / self.num_channels;

        // Run the audio callback
        let ptr = self.thread.call(self.cb, &[Value::from(self.num_channels), Value::from(samples_per_chan)]);

        let mem_slice: &[i16] = self.thread.get_heap_slice_mut(ptr.as_usize(), output_len);
        out.copy_from_slice(&mem_slice);
    }
}

#[derive(Default)]
struct AudioState
{
    input_dev: Option<AudioDevice<AudioCB>>,
    output_dev: Option<AudioDevice<AudioCB>>,
}

// This is only accessed from the main thread
thread_local! {
    static AUDIO_STATE: RefCell<AudioState> = RefCell::new(AudioState::default());
}

pub fn audio_open_output(thread: &mut Thread, sample_rate: Value, num_channels: Value, format: Value, cb: Value) -> Value
{
    if thread.id != 0 {
        panic!("audio functions should only be called from the main thread");
    }

    AUDIO_STATE.with_borrow(|s| {
        if s.output_dev.is_some() {
            panic!("audio output device already open");
        }
    });

    let sample_rate = sample_rate.as_u32();
    let num_channels = num_channels.as_u16();
    let format = format.as_u16();
    let cb = cb.as_u64();

    if sample_rate != 44100 {
        panic!("for now, only 44100Hz sample rate suppored");
    }

    //if num_channels > 2 {
    if num_channels != 1 {
        panic!("for now, only one output channel supported");
    }

    if format != AUDIO_FORMAT_I16 {
        panic!("for now, only i16, 16-bit signed audio format supported");
    }

    let desired_spec = AudioSpecDesired {
        freq: Some(sample_rate as i32),
        channels: Some(num_channels as u8),
        samples: Some(1024) // buffer size, 1024 samples
    };

    // Create a new VM thread in which to run the audio callback
    let audio_thread = VM::new_thread(&thread.vm);

    let sdl = get_sdl_context();
    let audio_subsystem = sdl.audio().unwrap();

    let device = audio_subsystem.open_playback(None, &desired_spec, |spec| {
        // initialize the audio callback
        AudioCB {
            num_channels: num_channels.into(),
            thread: audio_thread,
            cb: cb,
        }
    }).unwrap();

    // Start playback
    device.resume();

    // Keep the audio device alive
    AUDIO_STATE.with_borrow_mut(|s| {
        s.output_dev = Some(device);
    });

    // FIXME: return the device_id (u32)
    Value::from(0)
}

pub fn audio_open_input(thread: &mut Thread, sample_rate: Value, num_channels: Value, format: Value, cb: Value) -> Value
{
    if thread.id != 0 {
        panic!("audio functions should only be called from the main thread");
    }

    AUDIO_STATE.with_borrow(|s| {
        if s.input_dev.is_some() {
            panic!("audio input device already open");
        }
    });

    let sample_rate = sample_rate.as_u32();
    let num_channels = num_channels.as_u16();
    let format = format.as_u16();
    let cb = cb.as_u64();

    if sample_rate != 44100 {
        panic!("for now, only 44100Hz sample rate suppored");
    }

    //if num_channels > 2 {
    if num_channels != 1 {
        panic!("for now, only one output channel supported");
    }

    if format != AUDIO_FORMAT_I16 {
        panic!("for now, only i16, 16-bit signed audio format supported");
    }

    let desired_spec = AudioSpecDesired {
        freq: Some(sample_rate as i32),
        channels: Some(num_channels as u8),
        samples: Some(1024) // buffer size, 1024 samples
    };

    // Create a new VM thread in which to run the audio callback
    let audio_thread = VM::new_thread(&thread.vm);

    let sdl = get_sdl_context();
    let audio_subsystem = sdl.audio().unwrap();

    let device = audio_subsystem.open_capture(None, &desired_spec, |spec| {
        // initialize the audio callback
        AudioCB {
            num_channels: num_channels.into(),
            thread: audio_thread,
            cb: cb,
        }
    }).unwrap();

    // Start playback
    device.resume();

    // Keep the audio device alive
    AUDIO_STATE.with_borrow_mut(|s| {
        s.input_dev = Some(device);
    });

    // FIXME: return the device_id (u32)
    Value::from(1)
}
