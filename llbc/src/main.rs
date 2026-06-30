#![allow(dead_code)]

mod ast;
mod lexer;
mod parser;

use std::process::exit;

use lexer::Lexer;
use parser::Parser;

fn main()
{
    let path = std::env::args()
        .nth(1)
        .unwrap_or_else(|| "tests/empty_main.ll".to_string());

    let lexer = match Lexer::from_file(&path) {
        Ok(l) => l,
        Err(e) => {
            eprintln!("{}", e);
            exit(1);
        }
    };

    let mut parser = Parser::new(lexer);
    let module = match parser.parse_module() {
        Ok(m) => m,
        Err(e) => {
            eprintln!("parse error: {}", e);
            exit(1);
        }
    };

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
