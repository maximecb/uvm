#![allow(unused_imports)]
#![allow(dead_code)]
#![allow(unused_variables)]
#![allow(unused_parens)]
#![allow(unused_mut)]

use std::fs;
use std::env;
use std::collections::HashSet;
use std::collections::HashMap;

// https://docs.rs/serde_json/latest/serde_json/
//use serde_json::json;
use serde::{Deserialize, Serialize};
use serde_json::Result;

/// Verify that a string is a valid ascii identifier
fn is_valid_ident(name: &str) -> bool
{
    if name.len() == 0 {
        return false;
    }

    if name != name.to_lowercase() {
        return false;
    }

    let ch0 = name.chars().nth(0).unwrap();
    if ch0 != '_' && !ch0.is_ascii_alphabetic() {
        return false;
    }

    for ch in name.chars() {
        if ch != '_' && !ch.is_ascii_alphanumeric() {
            return false;
        }
    }

    return true;
}





#[derive(Serialize, Deserialize, Debug)]
struct SysCall {
    name: String,
    args: Vec<String>,
    ret_type: String,
    permission: String,
    const_idx: Option<u16>,
    description: Option<String>,
}

#[derive(Serialize, Deserialize, Debug)]
struct SubSystem {
    subsystem: String,
    description: Option<String>,
    syscalls: Vec<SysCall>,
}










fn main()
{
    //let mut unique_names = HashSet::new();



    let syscalls_json = fs::read_to_string("syscalls.json").unwrap();

    let deserialized: Vec<SubSystem> = serde_json::from_str(&syscalls_json).unwrap();
    println!("deserialized = {:?}", deserialized);

    for subsystem in deserialized {
        if !is_valid_ident(&subsystem.subsystem) {
            panic!();
        }



        for syscall in subsystem.syscalls {
            if !is_valid_ident(&syscall.name) {
                panic!();
            }




        }
    }

    // TODO: check that all syscall names are unique

    // TODO: assign a unique id to each syscall









}
