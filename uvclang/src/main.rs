#![allow(dead_code)]

mod ast;
mod codegen;
mod frontend;
mod layout;
mod lexer;
mod parser;
mod syscalls;

use tempfile::{Builder, NamedTempFile};

use std::io::{Read, Seek, SeekFrom, Write};
use std::process::exit;

use codegen::Codegen;
use frontend::FrontendOpts;
use lexer::{Lexer, ParseError};
use parser::Parser;
use crate::frontend::{compile_to_ir, link_to_ir};

const USAGE: &str = "usage: uvclang <input.c|.ll> [-o out.asm] [-O0|-O1|-O2|-O3]\n       \
                     [-D<macro>] [-I<dir>] [-U<macro>] [-Wall|-W<warn>] [-std=<std>]\n       \
                     [-f<feature>] [--emit-ir] [--stats]";

/// URL for reporting LLVM IR uvclang cannot parse yet.
const ISSUE_URL: &str = "https://github.com/maximecb/uvm/issues/new";

static STDIO_C_CONTENT: &str = include_str!("../lib_src/stdio.c");

pub enum Source
{
    Snippet(String),
    FilePath(String),
}

pub struct SourceCompilation
{
    source_name: String,
    source : Source,
}

fn ir_source(source : &SourceCompilation, fe : FrontendOpts) -> Result<NamedTempFile, String>
{
    if frontend::is_ir_path(&source.source_name) {
        // IR needs no clang step: hand llvm-link a temp file holding it.
        let mut ir_file = Builder::new()
            .suffix(".ll")
            .tempfile()
            .map_err(|e| e.to_string())?;
        match &source.source {
            Source::Snippet(snippet) => ir_file.write_all(snippet.as_bytes()),
            Source::FilePath(path) => std::fs::File::open(path)
                .and_then(|mut f| std::io::copy(&mut f, &mut ir_file).map(|_| ())),
        }
        .map_err(|e| e.to_string())?;
        Ok(ir_file)
    } else {
        let mut this_fe = fe.clone();
        this_fe.is_cpp = frontend::is_cpp_path(&source.source_name);
        match &source.source {
            Source::Snippet(snippet) => {
                let src_file = Builder::new()
                    .suffix(&source.source_name)
                    .tempfile()
                    .and_then(|mut f| {
                        writeln!(f, "{}", snippet)?;
                        Ok(f)
                    })
                    .map_err(|e| e.to_string())?;
                let tmp_file_name = src_file.path().to_str().ok_or("Path Error".to_string())?;
                let ir_result: NamedTempFile = frontend::compile_to_ir(tmp_file_name, &this_fe)?;
                Ok(ir_result)
            },
            Source::FilePath(path) => {
                let ir_result: NamedTempFile = frontend::compile_to_ir(&*path, &this_fe)?;
                Ok(ir_result)
            }
        }
    }
}

fn main()
{
    // Args: <input.c|.ll> [-o out.asm] [-O<n>] [-D..] [-I..] [--emit-ir] [--stats]
    // A `.ll` input is parsed directly (the back-end path, unchanged); any other
    // source is first lowered to IR by clang (the Phase 9 front-end).
    let args: Vec<String> = std::env::args().collect();
    let mut inputs: Vec<String> = vec![];
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
                inputs.push(s.to_string());
            }
        }
        i += 1;
    }

    if inputs.is_empty() {
        eprintln!("{}", USAGE);
        exit(2);
    };

    let ir_files : Result<Vec<NamedTempFile>, String> = inputs.iter()
        .map(|input| {
            let mut this_fe = fe.clone();
            this_fe.is_cpp = frontend::is_cpp_path(&input);
            compile_to_ir(&input, &this_fe)
        })
        .collect();

    let ir_files: Vec<NamedTempFile> = match ir_files {
        Ok(a) => a,
        Err(e) => {
            eprintln!("module error: {}", e);
            print_unsupported_ir_hint();
            exit(1);
        }
    };

    let mut all_ir_files: Vec<NamedTempFile> = ir_files;

    // Compute runtime modules
    let runtime_c_compilations: Vec<SourceCompilation> = vec![
        SourceCompilation {
            source_name: "stdio.c".to_string(),
            source: Source::Snippet(STDIO_C_CONTENT.to_string()),
        }
    ];

    let runtime_c_ir_files: Vec<NamedTempFile>  = runtime_c_compilations.iter().map(
        | source_comp |  match ir_source(source_comp, fe.clone())
        {
            Ok(m) => m,
            Err(e) => {
                eprintln!("runtime compile error: {}", e);
                exit(1);
            }

        }
    ).collect();

    all_ir_files = all_ir_files.into_iter().chain(runtime_c_ir_files).collect();

    let all_ir_file_names : Vec<String> = all_ir_files.iter().map(
        |named_temp | named_temp.path().to_string_lossy().into_owned()
    ).collect();

    let linked_ir: Result<String, String> = link_to_ir(&all_ir_file_names).and_then(
        |mut s| {
            s.seek(SeekFrom::Start(0)).expect("Can't go back to start of file.");
            let mut content = String::new();
            s.read_to_string(&mut content).map_err(|e| e.to_string())?;
            Ok(content)
        }
    );

    let linked_ir = match linked_ir {
        Ok(a) => a,
        Err(e) => {
            eprintln!("module error: {}", e);
            print_unsupported_ir_hint();
            exit(1);
        }
    };

    if emit_ir {
        emit(&out_path, linked_ir);
        return;
    }

    let linked_module = parse(&*linked_ir, "src_name");

    let linked_module = match linked_module {
        Ok(a) => a,
        Err(e) => {
            eprintln!("module error: {}", e);
            print_unsupported_ir_hint();
            exit(1);
        }
    };

    if stats {
        let joined_inputs = inputs.join(", ");
        print_stats(&joined_inputs, &linked_module);
        return;
    }

    let asm = match Codegen::new(&linked_module).generate() {
        Ok(a) => a,
        Err(e) => {
            eprintln!("codegen error: {}", e);
            print_unsupported_ir_hint();
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

/// Print guidance shown when uvclang fails to handle a piece of LLVM IR, whether
/// the failure surfaced in the parser or later in codegen. Both cases usually mean
/// the IR uses a construct uvclang does not support yet.
fn print_unsupported_ir_hint()
{
    eprintln!(
        "\nuvclang could not compile this LLVM IR. This usually means the IR uses \
         a construct uvclang does not support yet.\n\
         - If your clang is out of date, please update it and try again.\n\
         - Please also consider opening an issue on the UVM repo so we can add \
         support for it: {}",
        ISSUE_URL
    );
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
