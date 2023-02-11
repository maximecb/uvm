use std::time::{SystemTime, UNIX_EPOCH};
use crate::vm::{VM, Value};

// Callback function to be run at a given time stamp
#[derive(Debug, Copy, Clone)]
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

    // Add the new callback to the list
    let time_state = &mut vm.sys_state.time_state;
    time_state.delay_cbs.push(cb_entry);

    // Sort the callbacks by decreasing trigger time
    //time_state.delay_cbs.sort_by(|a, b| b.time_ms.cmp(&a.time_ms));
}

/// Compute the time untl the next delay callback needs to run
pub fn time_until_next_cb(vm: &VM) -> Option<u64>
{
    let time_state = &vm.sys_state.time_state;

    let last_cb = time_state.delay_cbs.last();

    match last_cb {
        None => return None,
        Some(cb) => {
            let cb_time = cb.time_ms;
            let cur_time = get_time_ms();
            let time_to = if cb_time > cur_time { cb_time - cur_time } else { 0 };
            return Some(time_to);
        }
    }
}

/// Get the list of PCs for callbacks to be run now
pub fn get_cbs_to_run(vm: &mut VM) -> Vec<u64>
{
    let time_state = &mut vm.sys_state.time_state;

    // Extract callbacks to run and extract the PCs
    let cur_time_ms = get_time_ms();
    let cbs_to_run = time_state.delay_cbs.iter().filter(|cb| cb.time_ms <= cur_time_ms);
    let pcs_to_run = cbs_to_run.map(|cb| cb.pc).collect();

    // Remove the callbacks to run from the list
    time_state.delay_cbs.retain(|cb| cb.time_ms > cur_time_ms);

    return pcs_to_run;
}
