// Blocking audio API.
//
// The VM does NOT run guest code on an audio thread. Instead each device is a
// small rendezvous/queue between SDL's real-time audio callback (which cannot
// block) and blocking syscalls the guest drives from its own thread(s):
//
//   output:  audio_wait_output(dev)  -> blocks until SDL needs the next buffer
//            audio_write(dev, buf, n) -> hands SDL that buffer (copied in)
//   input:   audio_read(dev, buf, n)  -> blocks until n frames are captured
//   both:    audio_close(dev)         -> stops the device, unblocks waiters
//
// Output uses a just-in-time rendezvous (fill on demand) so latency stays near
// one buffer period; the callback waits for the guest's audio_write up to one
// period, then falls back to silence so SDL's audio thread never stalls.

use sdl2::audio::{AudioCallback, AudioSpecDesired, AudioDevice};
use std::sync::{Arc, Mutex, Condvar};
use std::collections::VecDeque;
use std::time::Duration;
use std::cell::RefCell;
use crate::vm::{Value, Thread};
use crate::host::get_sdl_context;
use crate::constants::*;

// Fixed device parameters (unchanged from the previous API).
const BUF_FRAMES: usize = 1024;     // SDL period, in frames (samples per channel)
const SAMPLE_RATE: u32 = 44100;

// Device ids handed back to the guest. Only one input and one output device can
// be open at a time, so the ids are fixed.
const DEV_OUTPUT: u32 = 0;
const DEV_INPUT: u32 = 1;

// -- output ---------------------------------------------------------------

enum OutPhase {
    Idle,       // SDL is not currently asking for a buffer
    Requested,  // SDL wants a buffer and is waiting for audio_write
}

struct OutputShared {
    phase: OutPhase,
    pending: Option<Vec<i16>>,  // buffer supplied by audio_write, awaiting the callback
    open: bool,
    channels: usize,
}

struct OutputDev {
    m: Mutex<OutputShared>,
    cv: Condvar,
}

// SDL playback callback: request a buffer, wait (bounded) for the guest to
// supply one via audio_write, output it (silence on timeout/underrun/close).
struct OutputCB {
    dev: Arc<OutputDev>,
}

impl AudioCallback for OutputCB {
    type Channel = i16;

    fn callback(&mut self, out: &mut [i16]) {
        let mut s = self.dev.m.lock().unwrap();

        if !s.open {
            out.fill(0);
            return;
        }

        // Ask the guest for the next buffer and wake audio_wait_output.
        s.phase = OutPhase::Requested;
        self.dev.cv.notify_all();

        // Wait for audio_write to deliver, up to ~one buffer period.
        let deadline = Duration::from_micros(1_000_000 * BUF_FRAMES as u64 / SAMPLE_RATE as u64);
        loop {
            if let Some(buf) = s.pending.take() {
                let n = buf.len().min(out.len());
                out[..n].copy_from_slice(&buf[..n]);
                if n < out.len() { out[n..].fill(0); }
                break;
            }
            if !s.open {
                out.fill(0);
                break;
            }
            let (g, timed_out) = self.dev.cv.wait_timeout(s, deadline).unwrap();
            s = g;
            if timed_out.timed_out() && s.pending.is_none() {
                out.fill(0);   // underrun: guest was too slow this period
                break;
            }
        }

        s.phase = OutPhase::Idle;
    }
}

// -- input ----------------------------------------------------------------

struct InputShared {
    queue: VecDeque<i16>,
    open: bool,
    channels: usize,
}

struct InputDev {
    m: Mutex<InputShared>,
    cv: Condvar,
}

// SDL capture callback: append captured samples to the queue (dropping the
// oldest if the guest is not reading fast enough), then wake audio_read.
struct InputCB {
    dev: Arc<InputDev>,
}

impl AudioCallback for InputCB {
    type Channel = i16;

    fn callback(&mut self, buf: &mut [i16]) {
        let mut s = self.dev.m.lock().unwrap();
        if !s.open {
            return;
        }
        s.queue.extend(buf.iter().copied());
        // Bound the backlog to a few periods; drop oldest on overflow.
        let cap = 4 * BUF_FRAMES * s.channels;
        while s.queue.len() > cap {
            s.queue.pop_front();
        }
        self.dev.cv.notify_all();
    }
}

// -- registry -------------------------------------------------------------
//
// The Arc<{Output,Input}Dev> is shared with the guest syscalls (any thread) via
// these globals. The SDL AudioDevice itself keeps the callback running and is
// held in a main-thread-only cell (audio_open is main-thread only); it is not
// Send, so it never crosses to another thread.

static OUTPUT: Mutex<Option<Arc<OutputDev>>> = Mutex::new(None);
static INPUT: Mutex<Option<Arc<InputDev>>> = Mutex::new(None);

thread_local! {
    // Keeps the SDL devices alive for the life of the program (accessed only on
    // the main thread, where audio_open_* runs).
    static DEVICES: RefCell<AudioDevices> = RefCell::new(AudioDevices::default());
}

#[derive(Default)]
struct AudioDevices {
    output: Option<AudioDevice<OutputCB>>,
    input: Option<AudioDevice<InputCB>>,
}

fn output_shared() -> Option<Arc<OutputDev>> {
    OUTPUT.lock().unwrap().clone()
}

fn input_shared() -> Option<Arc<InputDev>> {
    INPUT.lock().unwrap().clone()
}

fn check_open_args(thread: &Thread, sample_rate: u32, num_channels: u16, format: u16) {
    if thread.id != 0 {
        panic!("audio devices must be opened from the main thread");
    }
    if sample_rate != SAMPLE_RATE {
        panic!("for now, only a {}Hz sample rate is supported", SAMPLE_RATE);
    }
    // Interleaved samples, left channel first (e.g. L R L R for stereo).
    if num_channels != 1 && num_channels != 2 {
        panic!("for now, only 1 or 2 audio channels are supported");
    }
    if format != AUDIO_FORMAT_I16 {
        panic!("for now, only the i16 (16-bit signed) audio format is supported");
    }
}

// -- syscalls -------------------------------------------------------------

pub fn audio_open_output(thread: &mut Thread, sample_rate: Value, num_channels: Value, format: Value) -> Value {
    check_open_args(thread, sample_rate.as_u32(), num_channels.as_u16(), format.as_u16());
    let num_channels = num_channels.as_u16() as usize;

    if OUTPUT.lock().unwrap().is_some() {
        panic!("audio output device already open");
    }

    let dev = Arc::new(OutputDev {
        m: Mutex::new(OutputShared {
            phase: OutPhase::Idle,
            pending: None,
            open: true,
            channels: num_channels,
        }),
        cv: Condvar::new(),
    });

    let desired = AudioSpecDesired {
        freq: Some(SAMPLE_RATE as i32),
        channels: Some(num_channels as u8),
        samples: Some(BUF_FRAMES as u16),
    };

    let sdl = get_sdl_context();
    let audio = sdl.audio().unwrap();
    let cb_dev = dev.clone();
    let device = audio.open_playback(None, &desired, |_spec| OutputCB { dev: cb_dev }).unwrap();
    device.resume();

    *OUTPUT.lock().unwrap() = Some(dev);
    DEVICES.with_borrow_mut(|d| d.output = Some(device));

    Value::from(DEV_OUTPUT)
}

pub fn audio_wait_output(_thread: &mut Thread, _device_id: Value) {
    let dev = match output_shared() { Some(d) => d, None => return };
    let mut s = dev.m.lock().unwrap();
    // Block until SDL asks for a buffer (or the device is closed).
    while s.open && !matches!(s.phase, OutPhase::Requested) {
        s = dev.cv.wait(s).unwrap();
    }
}

pub fn audio_write(thread: &mut Thread, _device_id: Value, samples_ptr: Value, num_frames: Value) {
    let dev = match output_shared() { Some(d) => d, None => return };
    let channels = { dev.m.lock().unwrap().channels };
    let n = num_frames.as_usize() * channels;
    let src: &[i16] = thread.get_heap_slice_mut(samples_ptr.as_usize(), n);
    let buf: Vec<i16> = src.to_vec();

    let mut s = dev.m.lock().unwrap();
    s.pending = Some(buf);
    dev.cv.notify_all();
}

pub fn audio_open_input(thread: &mut Thread, sample_rate: Value, num_channels: Value, format: Value) -> Value {
    check_open_args(thread, sample_rate.as_u32(), num_channels.as_u16(), format.as_u16());
    let num_channels = num_channels.as_u16() as usize;

    if INPUT.lock().unwrap().is_some() {
        panic!("audio input device already open");
    }

    let dev = Arc::new(InputDev {
        m: Mutex::new(InputShared {
            queue: VecDeque::new(),
            open: true,
            channels: num_channels,
        }),
        cv: Condvar::new(),
    });

    let desired = AudioSpecDesired {
        freq: Some(SAMPLE_RATE as i32),
        channels: Some(num_channels as u8),
        samples: Some(BUF_FRAMES as u16),
    };

    let sdl = get_sdl_context();
    let audio = sdl.audio().unwrap();
    let cb_dev = dev.clone();
    let device = audio.open_capture(None, &desired, |_spec| InputCB { dev: cb_dev }).unwrap();
    device.resume();

    *INPUT.lock().unwrap() = Some(dev);
    DEVICES.with_borrow_mut(|d| d.input = Some(device));

    Value::from(DEV_INPUT)
}

pub fn audio_read(thread: &mut Thread, _device_id: Value, samples_ptr: Value, num_frames: Value) {
    let dev = match input_shared() { Some(d) => d, None => return };
    let channels = { dev.m.lock().unwrap().channels };
    let need = num_frames.as_usize() * channels;

    let mut s = dev.m.lock().unwrap();
    while s.open && s.queue.len() < need {
        s = dev.cv.wait(s).unwrap();
    }
    let dst: &mut [i16] = thread.get_heap_slice_mut(samples_ptr.as_usize(), need);
    for slot in dst.iter_mut() {
        *slot = s.queue.pop_front().unwrap_or(0);
    }
}

pub fn audio_close(_thread: &mut Thread, device_id: Value) {
    match device_id.as_u32() {
        DEV_OUTPUT => {
            if let Some(dev) = OUTPUT.lock().unwrap().take() {
                dev.m.lock().unwrap().open = false;
                dev.cv.notify_all();
            }
        }
        DEV_INPUT => {
            if let Some(dev) = INPUT.lock().unwrap().take() {
                dev.m.lock().unwrap().open = false;
                dev.cv.notify_all();
            }
        }
        _ => {}
    }
}
