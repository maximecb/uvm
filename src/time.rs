use std::time::{SystemTime, UNIX_EPOCH};
use crate::vm::{VM, Value};

// Callback function to be run at a given time stamp
struct DelayCb
{
    time_ms: u64,
    pc: u64,
}

pub struct TimeState
{
    // List of delay callbacks
    delay_cbs: Vec<DelayCb>,
}

impl TimeState
{
    pub fn new() -> Self
    {
        Self {
            delay_cbs: Vec::default(),
        }
    }
}

/// Get the current time stamp in milliseconds
pub fn get_time_ms() -> u64
{
    SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_millis() as u64
}

/// Get the current time stamp in milliseconds since the unix epoch
pub fn time_current_ms(vm: &mut VM) -> Value
{
    Value::from(get_time_ms())
}

/// Call a callback function after a given delay in milliseconds
pub fn time_delay_cb(vm: &mut VM, delay_ms: Value, callback_pc: Value)
{
    let delay_ms = delay_ms.as_u64();
    let callback_pc = callback_pc.as_u64();

    let time_ms = get_time_ms();

    let cb_entry = DelayCb {
        time_ms: time_ms + delay_ms,
        pc: callback_pc
    };



    // TODO
    //delay_cbs.push(cb_entry);

    // TODO: sort the callbacks






}
