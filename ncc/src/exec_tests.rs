#![cfg(test)]

use std::fs;
use std::io::{self, Write};
use std::process::Command;
use std::collections::HashSet;

fn compile_and_run(file_path: &str, parse_only: bool)
{
    if parse_only {
        io::stdout().write(format!("compiling and parsing: {}\n", file_path).as_bytes()).unwrap();
    }
    else
    {
        io::stdout().write(format!("compiling and running: {}\n", file_path).as_bytes()).unwrap();
    }
    io::stdout().flush().unwrap();

    // Compile the source file
    let mut command = Command::new("cargo");
    command.current_dir(".");
    command.arg("run");
    command.arg(file_path);
    println!("{:?}", command);
    let output = command.output().unwrap();
    assert!(output.status.success(), "compilation failed");

    // Run the compiled program
    let mut command = Command::new("cargo");
    command.current_dir("../vm");
    command.arg("run");
    command.arg("--");
    if parse_only { command.arg("--parse-only"); }
    command.arg("../ncc/out.asm");
    println!("{:?}", command);
    let output = command.output().unwrap();
    assert!(output.status.success(), "execution failed");
}

#[test]
fn exec_tests()
{
    // Some examples involve creating a UI window
    // We parse/validate those without executing them
    let mut parse_only = HashSet::new();
    parse_only.insert("ball.c");
    parse_only.insert("attackers.c");
    parse_only.insert("counter.c");
    parse_only.insert("gameoflife.c");
    parse_only.insert("paint.c");
    parse_only.insert("snake.c");

    for file in fs::read_dir("./examples").unwrap() {
        let file_path = file.unwrap().path();
        let file_name = file_path.file_name().unwrap().to_str().unwrap();
        let parse_only = parse_only.get(file_name).is_some();
        let file_path = file_path.display().to_string();
        compile_and_run(&file_path, parse_only);
    }

    for file in fs::read_dir("./tests").unwrap() {
        let file_path = file.unwrap().path().display().to_string();
        compile_and_run(&file_path, false);
    }
}
