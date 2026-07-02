//! The clang front-end (Phase 9): lower a C/C++ source file to textual LLVM IR
//! by invoking clang with uvclang's canonical flags, capturing the IR on stdout.
//! This is what makes uvclang a *C/C++* compiler rather than only an IR
//! back-end: the lexer/parser/codegen back-end then runs in-process on the
//! captured IR, with no temporary `.ll` written to disk.
//!
//! The flag set mirrors `tests/gen_ll.sh` exactly (target triple, the `-fno-*`
//! set, `-S -emit-llvm`, `-Iuvclang/include`) so a `uvclang foo.c` build and the
//! back-end's `.ll`-driven test path produce identical IR.

use std::path::Path;
use std::process::Command;

/// Options controlling the clang invocation, gathered from the uvclang CLI.
pub struct FrontendOpts
{
    /// Optimization level flag, e.g. `-O2` (the default; overridable on the CLI).
    pub opt_level: String,
    /// Extra clang args forwarded verbatim, e.g. `-DFOO=1`, `-I/some/dir`.
    pub passthrough: Vec<String>,
    /// C++ source (chosen by extension) → drive `clang++` instead of `clang`.
    pub is_cpp: bool,
}

impl Default for FrontendOpts
{
    fn default() -> Self
    {
        Self {
            opt_level: "-O2".to_string(),
            passthrough: Vec::new(),
            is_cpp: false,
        }
    }
}

/// C++ source extensions (case-sensitive; bare `.C` is C++ by convention). Any
/// other non-`.ll` input is treated as C.
const CPP_EXTS: &[&str] = &["cpp", "cc", "cxx", "c++", "cp", "C", "CPP", "CXX", "CC"];

/// Does this path look like an LLVM IR file (parse it directly, skip clang)?
pub fn is_ir_path(path: &str) -> bool
{
    Path::new(path)
        .extension()
        .and_then(|e| e.to_str())
        .map(|e| e.eq_ignore_ascii_case("ll"))
        .unwrap_or(false)
}

/// Does this path look like a C++ source file?
pub fn is_cpp_path(path: &str) -> bool
{
    Path::new(path)
        .extension()
        .and_then(|e| e.to_str())
        .map(|e| CPP_EXTS.contains(&e))
        .unwrap_or(false)
}

/// The UVM-side include directory shipped with uvclang (headers such as
/// `<uvm/syscalls.h>`, `<string.h>`, ...). Resolved at compile time relative to
/// the crate so the built binary finds it regardless of the working directory;
/// override with `$UVCLANG_INCLUDE`.
fn uvm_include_dir() -> String
{
    if let Ok(dir) = std::env::var("UVCLANG_INCLUDE") {
        if !dir.is_empty() {
            return dir;
        }
    }
    concat!(env!("CARGO_MANIFEST_DIR"), "/include").to_string()
}

/// Resolve the clang binary the same way `gen_ll.sh` does: honor `$CLANG`
/// (`$CLANGXX` for C++), else prefer Homebrew LLVM (a common macOS dev setup),
/// else fall back to whatever `clang`/`clang++` is on PATH (Linux / CI).
fn find_clang(is_cpp: bool) -> String
{
    let (env_var, brew, base) = if is_cpp {
        ("CLANGXX", "/opt/homebrew/opt/llvm/bin/clang++", "clang++")
    } else {
        ("CLANG", "/opt/homebrew/opt/llvm/bin/clang", "clang")
    };

    if let Ok(c) = std::env::var(env_var) {
        if !c.is_empty() {
            return c;
        }
    }
    if Path::new(brew).exists() {
        return brew.to_string();
    }
    base.to_string()
}

/// Run clang on `source` and return the textual LLVM IR it emits on stdout.
pub fn compile_to_ir(source: &str, opts: &FrontendOpts) -> Result<String, String>
{
    let clang = find_clang(opts.is_cpp);

    let mut cmd = Command::new(&clang);
    cmd.arg("--target=x86_64-unknown-linux-gnu")
        .arg(&opts.opt_level)
        .arg(format!("-I{}", uvm_include_dir()))
        // Canonical uvclang flags (kept in lockstep with tests/gen_ll.sh):
        // keep value names for readable IR, and disable the transforms that
        // produce IR the back-end intentionally does not support.
        .arg("-fno-discard-value-names")
        .arg("-fno-optimize-sibling-calls")
        .arg("-fno-vectorize")
        .arg("-fno-slp-vectorize")
        .arg("-fno-jump-tables")
        .arg("-fno-strict-aliasing");

    // User -D / -I (and any other forwarded flags), after ours so the user can
    // override the built-in include search.
    for a in &opts.passthrough {
        cmd.arg(a);
    }

    cmd.arg("-S").arg("-emit-llvm").arg(source).arg("-o").arg("-");

    let output = match cmd.output() {
        Ok(o) => o,
        Err(e) => {
            return Err(format!(
                "could not run clang (\"{}\"): {}\n\
                 set $CLANG to a working clang, or install LLVM",
                clang, e
            ))
        }
    };

    if !output.status.success() {
        let stderr = String::from_utf8_lossy(&output.stderr);
        return Err(format!("clang failed:\n{}", stderr.trim_end()));
    }

    // Forward clang's diagnostics on a *successful* build too: with warning
    // flags like `-Wall` the point is to see the warnings, and `cmd.output()`
    // captured them rather than letting them reach the terminal.
    if !output.stderr.is_empty() {
        eprint!("{}", String::from_utf8_lossy(&output.stderr));
    }

    String::from_utf8(output.stdout).map_err(|e| format!("clang produced non-UTF8 IR: {}", e))
}
