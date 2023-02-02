#![cfg(test)]

use std::fs;
use std::io::{self, Write};
use std::process::Command;

fn compile_and_run(file_path: &str)
{
    io::stdout().write(file_path.as_bytes()).unwrap();
    io::stdout().write("\n".as_bytes()).unwrap();
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
    command.arg("../ncc/out.asm");
    println!("{:?}", command);
    let output = command.output().unwrap();
    assert!(output.status.success(), "execution failed");
}

#[test]
fn exec_tests()
{
    compile_and_run("examples/crc32.c");
    compile_and_run("examples/fib.c");
    compile_and_run("examples/fill_rect.c");
    compile_and_run("examples/random.c");
    compile_and_run("examples/strings.c");

    for file in fs::read_dir("./tests").unwrap() {
        let file_path = file.unwrap().path().display().to_string();
        compile_and_run(&file_path);
    }
}
