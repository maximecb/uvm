use sdl2::audio::{AudioCallback, AudioSpecDesired, AudioDevice};
use std::sync::{Arc, Weak, Mutex};
use crate::vm::{Value, VM, Thread};
use crate::sys::{get_sdl_context};
use crate::sys::constants::*;

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

// TODO: support multiple audio devices
/// We have to keep the audio device alive
/// This is a global variable because it doesn't implement
/// the Send trait, and so can't be referenced from another thread
static mut DEVICE: Option<AudioDevice<AudioCB>> = None;

// NOTE: this can only be called from the main thread since it uses SDL
// However, it creates a new thread to generate audio sample, this thread
// could be given a reference to another VM instance
pub fn audio_open_output(thread: &mut Thread, sample_rate: Value, num_channels: Value, format: Value, cb: Value) -> Value
{
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

    let sdl = get_sdl_context();
    let audio_subsystem = sdl.audio().unwrap();

    let desired_spec = AudioSpecDesired {
        freq: Some(sample_rate as i32),
        channels: Some(num_channels as u8), // mono
        samples: Some(1024) // buffer size, 1024 samples
    };

    // Create a new VM thread in which to run the audio callback
    let audio_thread = VM::new_thread(&thread.vm);

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
    unsafe {
        DEVICE = Some(device);
    }

    // TODO: return the device_id (u32)
    Value::from(0)
}
