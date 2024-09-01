use std::time::{SystemTime, UNIX_EPOCH};
use crate::vm::{Thread, Value};

/// Get the current time stamp in milliseconds
pub fn get_time_ms() -> u64
{
    SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_millis() as u64
}

/// Get the current time stamp in milliseconds since the unix epoch
pub fn time_current_ms(thread: &mut Thread) -> Value
{
    Value::from(get_time_ms())
}
