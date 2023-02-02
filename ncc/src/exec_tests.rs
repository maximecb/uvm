#![cfg(test)]

use std::fs;
use std::process::Command;

fn compile_and_run(file_path: &str)
{
    dbg!("{}", file_path);

    // Compile the source file
    let mut command = Command::new("cargo");
    command.current_dir(".");
    command.arg("run");
    command.arg(file_path);
    command.output().expect("compilation failed");

    println!("compiled");

    // Run the compiled program
    let mut command = Command::new("cargo");
    command.current_dir("../vm");
    command.arg("run");
    command.arg("ncc/out.asm");
    command.output().expect("execution failed");

    println!("executed");
}

#[test]
fn exec_examples()
{
    compile_and_run("examples/crc32.c");
    compile_and_run("examples/fib.c");
    compile_and_run("examples/fill_rect.c");
    compile_and_run("examples/random.c");
    compile_and_run("examples/strings.c");
}

#[test]
fn exec_tests()
{
    for file in fs::read_dir("./tests").unwrap() {
        let file_path = file.unwrap().path().display().to_string();
        compile_and_run(&file_path);
    }

    //assert!(false);
}
