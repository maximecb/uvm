#![allow(unused_imports)]
#![allow(dead_code)]
#![allow(unused_variables)]
#![allow(unused_parens)]
#![allow(unused_mut)]

use std::fs;
use std::fs::File;
use std::io::Write;
use std::env;
use std::collections::HashSet;
use std::collections::HashMap;

// https://docs.rs/serde_json/latest/serde_json/
//use serde_json::json;
use serde::{Deserialize, Serialize};
use serde_json::Result;

#[derive(Serialize, Deserialize, Debug, Clone)]
struct SysCall {
    name: String,
    args: Vec<(String, String)>,
    returns: (String, String),
    permission: String,
    const_idx: Option<u16>,
    description: Option<String>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
struct SubSystem {
    subsystem: String,
    description: Option<String>,
    syscalls: Vec<SysCall>,
}

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

fn gen_rust_bindings(out_file: &str, subsystems: &Vec<SubSystem>)
{
    // Generate syscall constants in rust
    let mut file = File::create(out_file).unwrap();
    writeln!(&mut file, "// This file was automatically generated based on syscalls.json").unwrap();
    writeln!(&mut file).unwrap();

    writeln!(&mut file, "#![allow(unused)]").unwrap();
    for subsystem in subsystems {
        for syscall in &subsystem.syscalls {
            let name = syscall.name.to_uppercase();
            let idx = syscall.const_idx.unwrap();
            writeln!(&mut file, "const {}: u16 = {};", name, idx).unwrap();
        }
    }

    // Generate global array of syscall descriptors
    writeln!(&mut file).unwrap();
    writeln!(&mut file, "{}", concat!(
        "struct SysCallDesc\n",
        "{\n",
        "    name: &'static str,\n",
        "    const_idx: u16,\n",
        "    argc: usize,\n",
        "}",
    )).unwrap();
    writeln!(&mut file).unwrap();

    // Generate an array of syscalls sorted by const_idx
    let mut syscall_list: Vec<SysCall> = Vec::new();
    for subsystem in subsystems {
        for syscall in &subsystem.syscalls {
            syscall_list.push(syscall.clone());
        }
    }
    syscall_list.sort_by(|a, b| a.const_idx.unwrap().cmp(&b.const_idx.unwrap()));

    writeln!(&mut file, "const SYSCALL_DESCS: [SysCallDesc; {}] = [", syscall_list.len()).unwrap();
    for syscall in syscall_list {
        writeln!(
            &mut file,
            "    SysCallDesc {{ name: \"{}\", const_idx: {}, argc: {} }},",
            syscall.name,
            syscall.const_idx.unwrap(),
            syscall.args.len(),
        ).unwrap();
    }
    writeln!(&mut file, "];").unwrap();
}

fn gen_c_bindings(out_file: &str, subsystems: &Vec<SubSystem>)
{
    // Generate C bindings
    let mut file = File::create(out_file).unwrap();
    writeln!(&mut file, "//").unwrap();
    writeln!(&mut file, "// This file was automatically generated based on syscalls.json").unwrap();
    writeln!(&mut file, "//").unwrap();
    writeln!(&mut file).unwrap();

    for subsystem in subsystems {
        for syscall in &subsystem.syscalls {
            // Add description comment if present
            if let Some(text) = &syscall.description {
                writeln!(&mut file, "// {}", text).unwrap();
            }

            //let name = syscall.name.to_uppercase();
            let fn_name = syscall.name.clone();
            let const_idx = syscall.const_idx.unwrap();

            // Function arguments
            let mut arg_str = "".to_string();
            for (idx, arg) in syscall.args.iter().enumerate() {
                if idx > 0 {
                    arg_str += ", ";
                }
                arg_str += &format!("{} {}", arg.0, arg.1);
            }
            //println!("{}", arg_str);

            let mut sys_arg_str = "".to_string();
            for (idx, arg) in syscall.args.iter().enumerate() {
                if idx > 0 {
                    sys_arg_str += ", ";
                }
                sys_arg_str += &arg.1;
            }
            //println!("{}", sys_arg_str);

            writeln!(&mut file,
                "{} {}({})\n{{\n    return syscall ({}) -> {} {{ syscall {}; }};\n}}\n",
                syscall.returns.0,
                fn_name,
                arg_str,
                sys_arg_str,
                syscall.returns.0,
                const_idx,
            ).unwrap();
        }
    }
}

fn main()
{
    let mut unique_names: HashSet<String> = HashSet::new();

    // Map from constant index to name
    let mut idx_to_name: Vec<Option<String>> = Vec::default();

    let syscalls_json = fs::read_to_string("syscalls.json").unwrap();
    let mut subsystems: Vec<SubSystem> = serde_json::from_str(&syscalls_json).unwrap();
    //println!("deserialized = {:?}", deserialized);

    // For each subsystem
    for subsystem in &subsystems {
        if !is_valid_ident(&subsystem.subsystem) {
            panic!();
        }

        // For each syscall for this subsystem
        for syscall in &subsystem.syscalls {
            // Make sure that syscall names are valid
            if !is_valid_ident(&syscall.name) {
                panic!();
            }

            // Make sure that syscall names are unique
            if unique_names.get(&syscall.name).is_some() {
                panic!();
            }
            unique_names.insert(syscall.name.clone());

            // Fill the map of indices to names
            if let Some(const_idx) = syscall.const_idx {
                let const_idx = const_idx as usize;
                if const_idx >= idx_to_name.len() {
                    idx_to_name.resize(const_idx + 1, None);
                }

                if idx_to_name[const_idx].is_some() {
                    panic!();
                }

                idx_to_name[const_idx] = Some(syscall.name.clone());
            }
        }
    }

    // Verify that there are no gaps in the syscall indices,
    // that is, every syscall idx up to the maximun index is taken
    for (idx, maybe_name) in idx_to_name.iter().enumerate() {
        if maybe_name.is_none() {
            panic!();
        }
    }

    // Allocate new indices to the syscalls that don't have indices yet
    for mut subsystem in &mut subsystems {
        for syscall in &mut subsystem.syscalls {
            if syscall.const_idx.is_none() {
                let const_idx = idx_to_name.len() as u16;
                syscall.const_idx = Some(const_idx);
                idx_to_name.push(Some(syscall.name.clone()));
                println!("allocating const_idx={} to syscall \"{}\"", const_idx, syscall.name);
            }

        }
    }

    // Re-serialize the data and write it back to the JSON file
    let json_output = serde_json::to_string_pretty(&subsystems).unwrap();
    let mut file = File::create("syscalls.json").unwrap();
    file.write_all(json_output.as_bytes()).unwrap();

    // TODO: need some better output file names
    gen_rust_bindings("syscalls.rs", &subsystems);

    gen_c_bindings("syscalls.c", &subsystems);
}
