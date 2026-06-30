#![allow(dead_code)]

mod ast;
mod codegen;
mod layout;
mod lexer;
mod parser;

use std::process::exit;

use codegen::Codegen;
use lexer::{Lexer, ParseError};
use parser::Parser;

fn main()
{
    // Args: <input.ll> [-o <out.asm>] [--stats]
    let args: Vec<String> = std::env::args().collect();
    let mut input: Option<String> = None;
    let mut out_path: Option<String> = None;
    let mut stats = false;

    let mut i = 1;
    while i < args.len() {
        match args[i].as_str() {
            "--stats" => stats = true,
            "-o" => {
                i += 1;
                out_path = args.get(i).cloned();
            }
            s => {
                if input.is_none() {
                    input = Some(s.to_string());
                }
            }
        }
        i += 1;
    }

    let input = match input {
        Some(p) => p,
        None => {
            eprintln!("usage: llbc <input.ll> [-o <out.asm>] [--stats]");
            exit(2);
        }
    };

    let module = match parse(&input) {
        Ok(m) => m,
        Err(e) => {
            eprintln!("parse error: {}", e);
            exit(1);
        }
    };

    if stats {
        print_stats(&input, &module);
        return;
    }

    let asm = match Codegen::new(&module).generate() {
        Ok(a) => a,
        Err(e) => {
            eprintln!("codegen error: {}", e);
            exit(1);
        }
    };

    match out_path {
        Some(p) => {
            if let Err(e) = std::fs::write(&p, asm) {
                eprintln!("could not write {}: {}", p, e);
                exit(1);
            }
        }
        None => print!("{}", asm),
    }
}

fn parse(path: &str) -> Result<ast::Module, ParseError>
{
    let lexer = Lexer::from_file(path)?;
    Parser::new(lexer).parse_module()
}

fn print_stats(path: &str, module: &ast::Module)
{
    let defined = module.functions.iter().filter(|f| !f.is_decl()).count();
    let declared = module.functions.iter().filter(|f| f.is_decl()).count();
    let total_blocks: usize = module.functions.iter().map(|f| f.blocks.len()).sum();
    let total_insts: usize = module
        .functions
        .iter()
        .flat_map(|f| f.blocks.iter())
        .map(|bb| bb.insts.len())
        .sum();

    println!("parsed {}", path);
    println!("  source_filename:    {:?}", module.source_filename);
    println!("  target triple:      {:?}", module.target_triple);
    println!("  struct types:       {}", module.struct_types.len());
    println!("  globals:            {}", module.globals.len());
    println!("  functions defined:  {}", defined);
    println!("  functions declared: {}", declared);
    println!("  basic blocks:       {}", total_blocks);
    println!("  non-term insts:     {}", total_insts);
}
