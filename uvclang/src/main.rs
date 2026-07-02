#![allow(dead_code)]

mod ast;
mod codegen;
mod frontend;
mod layout;
mod lexer;
mod parser;

use std::process::exit;

use codegen::Codegen;
use frontend::FrontendOpts;
use lexer::{Lexer, ParseError};
use parser::Parser;

const USAGE: &str = "usage: uvclang <input.c|.ll> [-o out.asm] [-O0|-O1|-O2|-O3]\n       \
                     [-D<macro>] [-I<dir>] [-U<macro>] [-Wall|-W<warn>] [-std=<std>]\n       \
                     [-f<feature>] [--emit-ir] [--stats]";

fn main()
{
    // Args: <input.c|.ll> [-o out.asm] [-O<n>] [-D..] [-I..] [--emit-ir] [--stats]
    // A `.ll` input is parsed directly (the back-end path, unchanged); any other
    // source is first lowered to IR by clang (the Phase 9 front-end).
    let args: Vec<String> = std::env::args().collect();
    let mut input: Option<String> = None;
    let mut out_path: Option<String> = None;
    let mut stats = false;
    let mut emit_ir = false;
    let mut fe = FrontendOpts::default();

    let mut i = 1;
    while i < args.len() {
        let a = args[i].as_str();
        match a {
            "--stats" => stats = true,
            "--emit-ir" | "--emit-llvm" => emit_ir = true,
            "-o" => {
                i += 1;
                out_path = args.get(i).cloned();
            }
            // Optimization level: forwarded to clang; ignored for `.ll` input.
            "-O0" | "-O1" | "-O2" | "-O3" | "-Os" | "-Oz" => fe.opt_level = a.to_string(),
            // -D / -I / -U, both the joined (`-DFOO`) and split (`-D FOO`) forms.
            "-D" | "-I" | "-U" => {
                if let Some(v) = args.get(i + 1) {
                    fe.passthrough.push(format!("{}{}", a, v));
                    i += 1;
                }
            }
            _ if a.starts_with("-D") || a.starts_with("-I") || a.starts_with("-U") => {
                fe.passthrough.push(a.to_string());
            }
            // Common clang diagnostic / language / feature flags (e.g. `-Wall`,
            // `-std=c11`, `-ffast-math`) forwarded verbatim to the front-end.
            _ if is_clang_passthrough(a) => fe.passthrough.push(a.to_string()),
            _ if a.starts_with('-') => {
                eprintln!("uvclang: unknown option \"{}\"\n{}", a, USAGE);
                exit(2);
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
            eprintln!("{}", USAGE);
            exit(2);
        }
    };

    // Front-end: obtain textual LLVM IR either straight from a `.ll` file or by
    // driving clang on a C/C++ source in-process (no temp `.ll` on disk).
    let (ir, ir_name) = if frontend::is_ir_path(&input) {
        match std::fs::read_to_string(&input) {
            Ok(src) => (src, input.clone()),
            Err(e) => {
                eprintln!("could not read input file \"{}\": {}", input, e);
                exit(1);
            }
        }
    } else {
        fe.is_cpp = frontend::is_cpp_path(&input);
        match frontend::compile_to_ir(&input, &fe) {
            Ok(ir) => (ir, input.clone()),
            Err(e) => {
                eprintln!("uvclang: {}", e);
                exit(1);
            }
        }
    };

    // --emit-ir: dump the front-end IR and stop (handy for growing C coverage).
    if emit_ir {
        emit(&out_path, ir);
        return;
    }

    let module = match parse(&ir, &ir_name) {
        Ok(m) => m,
        Err(e) => {
            eprintln!("parse error: {}", e);
            exit(1);
        }
    };

    if stats {
        print_stats(&ir_name, &module);
        return;
    }

    let asm = match Codegen::new(&module).generate() {
        Ok(a) => a,
        Err(e) => {
            eprintln!("codegen error: {}", e);
            exit(1);
        }
    };

    emit(&out_path, asm);
}

/// Common single-token clang flags uvclang forwards straight to the front-end:
/// warnings (`-Wall`, `-Wextra`, `-Werror`, `-Wno-...`), the language standard
/// (`-std=c11`, `-ansi`), and feature toggles (`-ffast-math`, `-fno-exceptions`).
/// None of these take a *separate* argument, so forwarding them can't desync arg
/// parsing the way `-o out` or `-D FOO` would; the split-argument flags stay
/// explicitly handled above. Forwarded flags land after uvclang's own canonical
/// flags, so a user flag overrides the matching default.
fn is_clang_passthrough(arg: &str) -> bool
{
    arg == "-w"
        || arg == "-ansi"
        || arg.starts_with("-W")
        || arg.starts_with("-f")
        || arg.starts_with("-std=")
        || arg.starts_with("-pedantic")
}

/// Write `text` to the `-o` path, or to stdout when none was given.
fn emit(out_path: &Option<String>, text: String)
{
    match out_path {
        Some(p) => {
            if let Err(e) = std::fs::write(p, text) {
                eprintln!("could not write {}: {}", p, e);
                exit(1);
            }
        }
        None => print!("{}", text),
    }
}

fn parse(src: &str, name: &str) -> Result<ast::Module, ParseError>
{
    let lexer = Lexer::new(src, name);
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
