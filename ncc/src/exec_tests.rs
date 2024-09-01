#![cfg(test)]

use std::fs;
use std::io::{self, Write};
use std::process::Command;
use std::collections::HashSet;

fn compile_and_run(file_path: &str, run_example: bool)
{
    if run_example {
        io::stdout().write(format!("compiling and running: {}\n", file_path).as_bytes()).unwrap();
    } else {
        io::stdout().write(format!("compiling and parsing: {}\n", file_path).as_bytes()).unwrap();
    }
    io::stdout().flush().unwrap();

    // Compile the source file
    let mut command = Command::new("target/debug/ncc");
    command.current_dir(".");
    command.arg("-DTEST");
    command.arg(file_path);
    println!("{:?}", command);
    let output = command.output().unwrap();
    assert!(output.status.success(), "compilation failed");

    // Run the compiled program
    let mut command = Command::new("target/debug/uvm");
    command.current_dir("../vm");
    if !run_example { command.arg("--parse-only"); }
    command.arg("../ncc/out.asm");
    println!("{:?}", command);
    let output = command.output().unwrap();
    assert!(output.status.success(), "execution failed");
}

#[test]
fn exec_tests()
{
    // Make sure that uvm is built in dev/debug mode
    let mut command = Command::new("cargo");
    command.current_dir("../vm");
    command.arg("build");
    command.arg("--profile");
    command.arg("dev");
    println!("{:?}", command);
    let output = command.output().unwrap();
    assert!(output.status.success(), "execution failed");

    // Make sure that ncc is built in dev/debug mode
    let mut command = Command::new("cargo");
    command.current_dir(".");
    command.arg("build");
    command.arg("--profile");
    command.arg("dev");
    println!("{:?}", command);
    let output = command.output().unwrap();
    assert!(output.status.success(), "execution failed");

    // Compile all the tests and run them
    for file in fs::read_dir("./tests").unwrap() {
        let file_path = file.unwrap().path().display().to_string();
        if file_path.ends_with(".c") {
            compile_and_run(&file_path, true);
        }
    }

    // We only run a subset of examples
    // Some examples involve creating a UI window
    // We parse/validate those without executing them
    let mut run_examples = HashSet::new();
    run_examples.insert("fib.c");
    run_examples.insert("crc32.c");
    run_examples.insert("inthash.c");
    run_examples.insert("helloworld.c");
    run_examples.insert("sdbm_hash.c");
    run_examples.insert("strings.c");

    // Compile the examples, but only run those that don't need a UI window
    for file in fs::read_dir("./examples").unwrap() {
        let file_path = file.unwrap().path();
        let file_name = file_path.file_name().unwrap().to_str().unwrap();
        if file_name.ends_with(".c") {
            let run_example = run_examples.get(file_name).is_some();
            let file_path = file_path.display().to_string();
            compile_and_run(&file_path, run_example);
        }
    }
}
