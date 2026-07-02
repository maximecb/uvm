//! Recursive-descent parser for the subset of textual LLVM IR emitted by
//! clang `-O2` (see tests/gen_ll.sh for the exact flags).
//!
//! The parser is deliberately permissive about *attributes*: anything that
//! doesn't affect code generation (linkage, parameter/function attributes,
//! `captures(...)`, metadata, ...) is recognized and discarded rather than
//! represented. See `skip_attrs`.

use std::mem;

use crate::ast::*;
use crate::lexer::{Lexer, ParseError};

/// No-argument attribute keywords that may appear in attribute positions
/// (linkage, visibility, parameter & function attributes). Consumed and
/// dropped. This list grows as new test inputs surface new attributes.
const FLAG_ATTRS: &[&str] = &[
    // linkage / visibility / preemption
    "private", "internal", "available_externally", "linkonce", "linkonce_odr",
    "weak", "weak_odr", "appending", "common", "extern_weak", "external",
    "hidden", "protected", "default", "dso_local", "dso_preemptable",
    "unnamed_addr", "local_unnamed_addr",
    // parameter attributes
    "noundef", "nonnull", "signext", "zeroext", "inreg", "noalias", "nocapture",
    "readonly", "readnone", "writeonly", "immarg", "returned", "nest", "writable",
    "dead_on_unwind", "dead_on_return",
    // function attributes
    "nounwind", "nofree", "mustprogress", "willreturn", "norecurse", "nosync",
    "nocallback", "uwtable", "noreturn", "noinline", "alwaysinline", "inlinehint",
    "optsize", "optnone", "minsize", "cold", "hot", "convergent", "speculatable",
    "nobuiltin", "noimplicitfloat", "noredzone", "strictfp", "ssp", "sspstrong",
    "sspreq", "sanitize_address", "sanitize_memory", "sanitize_thread",
];

/// Attribute keywords that take a parenthesized argument, e.g. `captures(none)`,
/// `memory(read)`, `byval(%struct.t)`, `range(i32 0, 10)`. The whole `(...)` is
/// skipped.
const PAREN_ATTRS: &[&str] = &[
    "captures", "memory", "range", "dereferenceable", "dereferenceable_or_null",
    "byval", "byref", "sret", "inalloca", "preallocated", "elementtype",
    "alignstack", "allocsize", "allockind", "vscale_range", "nofpclass",
    "addrspace", "initializes",
];

pub struct Parser
{
    input: Lexer,
}

impl Parser
{
    pub fn new(input: Lexer) -> Self
    {
        Parser { input }
    }

    // -----------------------------------------------------------------------
    // Top level
    // -----------------------------------------------------------------------

    pub fn parse_module(&mut self) -> Result<Module, ParseError>
    {
        let mut m = Module::default();

        loop {
            self.input.eat_ws()?;
            if self.input.eof() {
                break;
            }

            let c = self.input.peek_ch();

            if c == '@' {
                m.globals.push(self.parse_global_def()?);
                continue;
            }
            if c == '%' {
                let (name, body) = self.parse_type_def()?;
                m.struct_types.insert(name, body);
                continue;
            }
            if c == '!' {
                // Metadata definition / named metadata: irrelevant to codegen.
                self.skip_line();
                continue;
            }

            if self.input.match_keyword("source_filename")? {
                self.input.expect_token("=")?;
                m.source_filename = Some(self.parse_quoted()?);
                continue;
            }
            if self.input.match_keyword("target")? {
                if self.input.match_keyword("datalayout")? {
                    self.input.expect_token("=")?;
                    m.datalayout = Some(self.parse_quoted()?);
                } else if self.input.match_keyword("triple")? {
                    self.input.expect_token("=")?;
                    m.target_triple = Some(self.parse_quoted()?);
                } else {
                    return self.input.parse_error("expected 'datalayout' or 'triple'");
                }
                continue;
            }
            if self.input.match_keyword("define")? {
                m.functions.push(self.parse_define()?);
                continue;
            }
            if self.input.match_keyword("declare")? {
                m.functions.push(self.parse_declare()?);
                continue;
            }
            if self.input.match_keyword("attributes")? {
                // `attributes #N = { ... }` — a single line we don't care about.
                self.skip_line();
                continue;
            }

            return self.input.parse_error("unexpected top-level construct");
        }

        Ok(m)
    }

    // -----------------------------------------------------------------------
    // Globals and named types
    // -----------------------------------------------------------------------

    fn parse_global_def(&mut self) -> Result<Global, ParseError>
    {
        let name = self.parse_global_name()?;
        self.input.expect_token("=")?;
        self.skip_attrs()?; // linkage, unnamed_addr, ...

        let is_const = if self.input.match_keyword("constant")? {
            true
        } else if self.input.match_keyword("global")? {
            false
        } else {
            return self.input.parse_error("expected 'global' or 'constant'");
        };

        let ty = self.parse_type()?;

        self.input.eat_ws()?;
        let c = self.input.peek_ch();
        let init = if c == ',' || c == ';' || c == '\n' || c == '\0' {
            None
        } else {
            Some(self.parse_value()?)
        };

        // Trailing `, align N`, comdat, metadata attachments, ...
        self.skip_line();
        Ok(Global { name, ty, is_const, init })
    }

    fn parse_type_def(&mut self) -> Result<(String, Option<StructBody>), ParseError>
    {
        let name = self.parse_local_name()?;
        self.input.expect_token("=")?;
        if !self.input.match_keyword("type")? {
            return self.input.parse_error("expected 'type'");
        }
        if self.input.match_keyword("opaque")? {
            return Ok((name, None));
        }
        match self.parse_type()? {
            Type::Struct(body) => Ok((name, Some(body))),
            _ => self.input.parse_error("only struct type definitions are supported"),
        }
    }

    // -----------------------------------------------------------------------
    // Functions
    // -----------------------------------------------------------------------

    fn parse_define(&mut self) -> Result<Function, ParseError>
    {
        self.skip_attrs()?; // return-value attributes (dso_local, noundef, ...)
        let ret_ty = self.parse_type()?;
        let name = self.parse_global_name()?;
        self.input.expect_token("(")?;
        let (params, varargs) = self.parse_param_list()?;
        self.input.expect_token(")")?;
        self.skip_attrs()?; // function attributes (local_unnamed_addr #0 ...)
        self.input.expect_token("{")?;

        let blocks = self.parse_body()?;
        Ok(Function { name, ret_ty, params, varargs, blocks })
    }

    fn parse_declare(&mut self) -> Result<Function, ParseError>
    {
        self.skip_attrs()?;
        let ret_ty = self.parse_type()?;
        let name = self.parse_global_name()?;
        self.input.expect_token("(")?;
        let (params, varargs) = self.parse_param_list()?;
        self.input.expect_token(")")?;
        self.skip_line(); // trailing attributes
        Ok(Function { name, ret_ty, params, varargs, blocks: vec![] })
    }

    /// Parse a parenthesized parameter list (the opening `(` is already
    /// consumed; stops before the closing `)`).
    fn parse_param_list(&mut self) -> Result<(Vec<Param>, bool), ParseError>
    {
        let mut params = vec![];
        let mut varargs = false;

        self.input.eat_ws()?;
        if self.input.peek_ch() == ')' {
            return Ok((params, varargs));
        }

        loop {
            self.input.eat_ws()?;
            if self.input.match_token("...")? {
                varargs = true;
                break;
            }

            let ty = self.parse_type()?;
            self.skip_attrs()?; // parameter attributes

            self.input.eat_ws()?;
            let name = if self.input.peek_ch() == '%' {
                Some(self.parse_local_name()?)
            } else {
                None
            };
            params.push(Param { ty, name });

            if !self.input.match_token(",")? {
                break;
            }
        }

        Ok((params, varargs))
    }

    /// Parse a function body: a sequence of basic blocks up to the closing `}`.
    fn parse_body(&mut self) -> Result<Vec<BasicBlock>, ParseError>
    {
        let mut blocks = vec![];
        let mut label: Option<String> = None;
        let mut insts: Vec<Inst> = vec![];

        loop {
            self.input.eat_ws()?;
            let c = self.input.peek_ch();

            if c == '}' {
                self.input.eat_ch();
                break;
            }

            // Instruction with a result: `%dest = <op> ...`
            if c == '%' {
                let dest = self.parse_local_name()?;
                self.input.expect_token("=")?;
                let kind = self.parse_value_inst()?;
                insts.push(Inst { dest: Some(dest), kind });
                continue;
            }

            // Otherwise: a block label (`name:`) or a no-result instruction.
            let word = self.read_word();
            if word.is_empty() {
                return self.input.parse_error("unexpected character in function body");
            }
            if self.input.match_char(':') {
                label = Some(word);
                continue;
            }

            match word.as_str() {
                "ret" => {
                    let term = self.parse_ret()?;
                    blocks.push(finish_block(&mut label, &mut insts, term));
                }
                "br" => {
                    let term = self.parse_br()?;
                    blocks.push(finish_block(&mut label, &mut insts, term));
                }
                "switch" => {
                    let term = self.parse_switch()?;
                    blocks.push(finish_block(&mut label, &mut insts, term));
                }
                "unreachable" => {
                    self.parse_trailing()?;
                    blocks.push(finish_block(&mut label, &mut insts, Terminator::Unreachable));
                }
                "store" => {
                    let kind = self.parse_store()?;
                    insts.push(Inst { dest: None, kind });
                }
                "call" => {
                    let kind = self.parse_call()?;
                    insts.push(Inst { dest: None, kind });
                }
                "tail" | "musttail" | "notail" => {
                    self.expect_keyword("call")?;
                    let kind = self.parse_call()?;
                    insts.push(Inst { dest: None, kind });
                }
                other => {
                    return self
                        .input
                        .parse_error(&format!("instruction not yet supported: '{}'", other));
                }
            }
        }

        Ok(blocks)
    }

    /// Parse the right-hand side of a `%dest = ...` instruction.
    fn parse_value_inst(&mut self) -> Result<InstKind, ParseError>
    {
        self.input.eat_ws()?;
        let op = self.read_word();
        match op.as_str() {
            "add" => self.parse_bin(BinOp::Add),
            "sub" => self.parse_bin(BinOp::Sub),
            "mul" => self.parse_bin(BinOp::Mul),
            "udiv" => self.parse_bin(BinOp::UDiv),
            "sdiv" => self.parse_bin(BinOp::SDiv),
            "urem" => self.parse_bin(BinOp::URem),
            "srem" => self.parse_bin(BinOp::SRem),
            "and" => self.parse_bin(BinOp::And),
            "or" => self.parse_bin(BinOp::Or),
            "xor" => self.parse_bin(BinOp::Xor),
            "shl" => self.parse_bin(BinOp::Shl),
            "lshr" => self.parse_bin(BinOp::LShr),
            "ashr" => self.parse_bin(BinOp::AShr),
            "fadd" => self.parse_fbin(FBinOp::FAdd),
            "fsub" => self.parse_fbin(FBinOp::FSub),
            "fmul" => self.parse_fbin(FBinOp::FMul),
            "fdiv" => self.parse_fbin(FBinOp::FDiv),
            "frem" => self.parse_fbin(FBinOp::FRem),
            "fneg" => self.parse_fneg(),
            "trunc" => self.parse_conv(ConvOp::Trunc),
            "zext" => self.parse_conv(ConvOp::ZExt),
            "sext" => self.parse_conv(ConvOp::SExt),
            "ptrtoint" => self.parse_conv(ConvOp::PtrToInt),
            "inttoptr" => self.parse_conv(ConvOp::IntToPtr),
            "bitcast" => self.parse_conv(ConvOp::BitCast),
            "sitofp" => self.parse_conv(ConvOp::SIToFP),
            "uitofp" => self.parse_conv(ConvOp::UIToFP),
            "fptosi" => self.parse_conv(ConvOp::FPToSI),
            "fptoui" => self.parse_conv(ConvOp::FPToUI),
            "fpext" => self.parse_conv(ConvOp::FPExt),
            "fptrunc" => self.parse_conv(ConvOp::FPTrunc),
            "icmp" => self.parse_icmp(),
            "fcmp" => self.parse_fcmp(),
            "select" => self.parse_select(),
            "load" => self.parse_load(),
            "getelementptr" => self.parse_gep_inst(),
            "phi" => self.parse_phi(),
            "alloca" => self.parse_alloca(),
            "freeze" => self.parse_freeze(),
            "call" => self.parse_call(),
            "tail" | "musttail" | "notail" => {
                self.expect_keyword("call")?;
                self.parse_call()
            }
            other => self
                .input
                .parse_error(&format!("instruction not yet supported: '{}'", other)),
        }
    }

    // -- arithmetic / bitwise -------------------------------------------------

    fn parse_bin(&mut self, op: BinOp) -> Result<InstKind, ParseError>
    {
        self.skip_arith_flags()?; // nuw / nsw / exact / disjoint
        let ty = self.parse_type()?;
        let lhs = self.parse_value()?;
        self.input.expect_token(",")?;
        let rhs = self.parse_value()?;
        self.parse_trailing()?;
        Ok(InstKind::Bin { op, ty, lhs, rhs })
    }

    fn skip_arith_flags(&mut self) -> Result<(), ParseError>
    {
        loop {
            if self.input.match_keyword("nuw")?
                || self.input.match_keyword("nsw")?
                || self.input.match_keyword("exact")?
                || self.input.match_keyword("disjoint")?
            {
                continue;
            }
            break;
        }
        Ok(())
    }

    /// Consume any fast-math flags on a floating-point op (`fadd fast`, `fmul
    /// nnan ninf contract`, ...). They relax IEEE semantics but don't change our
    /// (already non-strict) lowering, so they are dropped.
    fn skip_fast_math_flags(&mut self) -> Result<(), ParseError>
    {
        const FLAGS: &[&str] =
            &["fast", "nnan", "ninf", "nsz", "arcp", "contract", "afn", "reassoc"];
        loop {
            let mut matched = false;
            for f in FLAGS {
                if self.input.match_keyword(f)? {
                    matched = true;
                    break;
                }
            }
            if !matched {
                break;
            }
        }
        Ok(())
    }

    fn parse_fbin(&mut self, op: FBinOp) -> Result<InstKind, ParseError>
    {
        self.skip_fast_math_flags()?;
        let ty = self.parse_type()?;
        let lhs = self.parse_value()?;
        self.input.expect_token(",")?;
        let rhs = self.parse_value()?;
        self.parse_trailing()?;
        Ok(InstKind::FBin { op, ty, lhs, rhs })
    }

    fn parse_fneg(&mut self) -> Result<InstKind, ParseError>
    {
        self.skip_fast_math_flags()?;
        let ty = self.parse_type()?;
        let val = self.parse_value()?;
        self.parse_trailing()?;
        Ok(InstKind::FNeg { ty, val })
    }

    fn parse_fcmp(&mut self) -> Result<InstKind, ParseError>
    {
        self.skip_fast_math_flags()?;
        let pred = self.parse_fcmp_pred()?;
        let ty = self.parse_type()?;
        let lhs = self.parse_value()?;
        self.input.expect_token(",")?;
        let rhs = self.parse_value()?;
        self.parse_trailing()?;
        Ok(InstKind::FCmp { pred, ty, lhs, rhs })
    }

    fn parse_fcmp_pred(&mut self) -> Result<FCmpPred, ParseError>
    {
        // Order matters: the multi-letter names must be tried before shorter
        // prefixes of them, but match_keyword is whole-token so any order works.
        let preds = [
            ("false", FCmpPred::False), ("oeq", FCmpPred::Oeq),
            ("ogt", FCmpPred::Ogt), ("oge", FCmpPred::Oge),
            ("olt", FCmpPred::Olt), ("ole", FCmpPred::Ole),
            ("one", FCmpPred::One), ("ord", FCmpPred::Ord),
            ("ueq", FCmpPred::Ueq), ("ugt", FCmpPred::Ugt),
            ("uge", FCmpPred::Uge), ("ult", FCmpPred::Ult),
            ("ule", FCmpPred::Ule), ("une", FCmpPred::Une),
            ("uno", FCmpPred::Uno), ("true", FCmpPred::True),
        ];
        for (kw, pred) in preds {
            if self.input.match_keyword(kw)? {
                return Ok(pred);
            }
        }
        self.input.parse_error("expected an fcmp predicate")
    }

    // -- conversions ----------------------------------------------------------

    fn parse_conv(&mut self, op: ConvOp) -> Result<InstKind, ParseError>
    {
        // Flags with no effect on our model: `zext nneg`, `trunc nuw nsw`.
        loop {
            if self.input.match_keyword("nneg")?
                || self.input.match_keyword("nuw")?
                || self.input.match_keyword("nsw")?
            {
                continue;
            }
            break;
        }
        let from_ty = self.parse_type()?;
        let val = self.parse_value()?;
        self.expect_keyword("to")?;
        let to_ty = self.parse_type()?;
        self.parse_trailing()?;
        Ok(InstKind::Conv { op, from_ty, val, to_ty })
    }

    // -- comparisons / select -------------------------------------------------

    fn parse_icmp(&mut self) -> Result<InstKind, ParseError>
    {
        self.input.match_keyword("samesign")?; // LLVM 20 flag, ignored
        let pred = self.parse_icmp_pred()?;
        let ty = self.parse_type()?;
        let lhs = self.parse_value()?;
        self.input.expect_token(",")?;
        let rhs = self.parse_value()?;
        self.parse_trailing()?;
        Ok(InstKind::ICmp { pred, ty, lhs, rhs })
    }

    fn parse_icmp_pred(&mut self) -> Result<ICmpPred, ParseError>
    {
        let preds = [
            ("eq", ICmpPred::Eq), ("ne", ICmpPred::Ne),
            ("ugt", ICmpPred::Ugt), ("uge", ICmpPred::Uge),
            ("ult", ICmpPred::Ult), ("ule", ICmpPred::Ule),
            ("sgt", ICmpPred::Sgt), ("sge", ICmpPred::Sge),
            ("slt", ICmpPred::Slt), ("sle", ICmpPred::Sle),
        ];
        for (kw, pred) in preds {
            if self.input.match_keyword(kw)? {
                return Ok(pred);
            }
        }
        self.input.parse_error("expected an icmp predicate")
    }

    fn parse_select(&mut self) -> Result<InstKind, ParseError>
    {
        let _cond_ty = self.parse_type()?; // i1
        let cond = self.parse_value()?;
        self.input.expect_token(",")?;
        let ty = self.parse_type()?;
        let tval = self.parse_value()?;
        self.input.expect_token(",")?;
        let _fty = self.parse_type()?;
        let fval = self.parse_value()?;
        self.parse_trailing()?;
        Ok(InstKind::Select { cond, ty, tval, fval })
    }

    // -- memory ---------------------------------------------------------------

    fn parse_load(&mut self) -> Result<InstKind, ParseError>
    {
        self.input.match_keyword("volatile")?;
        let ty = self.parse_type()?;
        self.input.expect_token(",")?;
        let _ptr_ty = self.parse_type()?; // ptr
        let ptr = self.parse_value()?;
        let align = self.parse_trailing()?;
        Ok(InstKind::Load { ty, ptr, align })
    }

    fn parse_store(&mut self) -> Result<InstKind, ParseError>
    {
        self.input.match_keyword("volatile")?;
        let ty = self.parse_type()?;
        let val = self.parse_value()?;
        self.input.expect_token(",")?;
        let _ptr_ty = self.parse_type()?; // ptr
        let ptr = self.parse_value()?;
        let align = self.parse_trailing()?;
        Ok(InstKind::Store { ty, val, ptr, align })
    }

    fn parse_alloca(&mut self) -> Result<InstKind, ParseError>
    {
        self.input.match_keyword("inalloca")?;
        let ty = self.parse_type()?;
        let mut count = None;
        let mut align = None;
        loop {
            if !self.input.match_token(",")? {
                break;
            }
            self.input.eat_ws()?;
            if self.input.match_keyword("align")? {
                self.input.eat_ws()?;
                align = Some(self.input.parse_int(10)? as u64);
            } else if self.input.peek_ch() == '!' {
                self.skip_metadata_ref()?;
            } else {
                count = Some(self.parse_typed_val()?);
            }
        }
        Ok(InstKind::Alloca { ty, count, align })
    }

    fn parse_gep_inst(&mut self) -> Result<InstKind, ParseError>
    {
        let inbounds = self.parse_gep_flags()?;
        let base_ty = self.parse_type()?;
        self.input.expect_token(",")?;
        let _ptr_ty = self.parse_type()?; // ptr
        let ptr = self.parse_value()?;
        let mut indices = vec![];
        loop {
            if !self.input.match_token(",")? {
                break;
            }
            self.input.eat_ws()?;
            if self.input.peek_ch() == '!' {
                self.skip_metadata_ref()?;
                continue;
            }
            indices.push(self.parse_typed_val()?);
        }
        Ok(InstKind::GetElementPtr { inbounds, base_ty, ptr, indices })
    }

    // -- phi / freeze / call --------------------------------------------------

    fn parse_phi(&mut self) -> Result<InstKind, ParseError>
    {
        let ty = self.parse_type()?;
        let mut incoming = vec![];
        loop {
            self.input.expect_token("[")?;
            let val = self.parse_value()?;
            self.input.expect_token(",")?;
            let label = self.parse_local_name()?;
            self.input.expect_token("]")?;
            incoming.push((val, label));
            if !self.input.match_token(",")? {
                break;
            }
        }
        Ok(InstKind::Phi { ty, incoming })
    }

    fn parse_freeze(&mut self) -> Result<InstKind, ParseError>
    {
        let ty = self.parse_type()?;
        let val = self.parse_value()?;
        self.parse_trailing()?;
        Ok(InstKind::Freeze { ty, val })
    }

    fn parse_call(&mut self) -> Result<InstKind, ParseError>
    {
        self.skip_call_cconv()?;
        self.skip_attrs()?; // return-value attributes
        let ret_ty = self.parse_type()?;

        // An explicit function (pointer) type may follow the return type,
        // e.g. `call void (...) %fp()` or `call i32 (ptr, ...) @printf(...)`.
        let mut fn_ty = None;
        self.input.eat_ws()?;
        if self.input.peek_ch() == '(' {
            self.input.expect_token("(")?;
            let (params, varargs) = self.parse_type_list()?;
            self.input.expect_token(")")?;
            fn_ty = Some(FuncType { ret: ret_ty.clone(), params, varargs });
        }

        let callee = self.parse_value()?;
        self.input.expect_token("(")?;
        let args = self.parse_arg_list()?;
        self.input.expect_token(")")?;
        self.skip_attrs()?; // function attributes (#N, ...)
        self.parse_trailing()?;
        Ok(InstKind::Call { ret_ty, fn_ty, callee, args })
    }

    /// Skip an optional calling convention preceding a call's return type.
    fn skip_call_cconv(&mut self) -> Result<(), ParseError>
    {
        const CCONVS: &[&str] = &[
            "ccc", "fastcc", "coldcc", "tailcc", "swiftcc", "swifttailcc",
            "anyregcc", "preserve_mostcc", "preserve_allcc", "cxx_fast_tlscc",
        ];
        for kw in CCONVS {
            if self.input.match_keyword(kw)? {
                return Ok(());
            }
        }
        if self.input.match_keyword("cc")? {
            self.input.eat_ws()?;
            self.input.parse_int(10)?;
        }
        Ok(())
    }

    /// Parse a comma-separated list of types (for an explicit function type),
    /// allowing a trailing `...` for varargs. Stops before the closing `)`.
    fn parse_type_list(&mut self) -> Result<(Vec<Type>, bool), ParseError>
    {
        let mut tys = vec![];
        let mut varargs = false;
        self.input.eat_ws()?;
        if self.input.peek_ch() == ')' {
            return Ok((tys, varargs));
        }
        loop {
            self.input.eat_ws()?;
            if self.input.match_token("...")? {
                varargs = true;
                break;
            }
            tys.push(self.parse_type()?);
            self.skip_attrs()?;
            if !self.input.match_token(",")? {
                break;
            }
        }
        Ok((tys, varargs))
    }

    /// Parse a call's argument list: `<ty> [attrs] <val>, ...`. Stops before
    /// the closing `)`.
    fn parse_arg_list(&mut self) -> Result<Vec<TypedVal>, ParseError>
    {
        let mut args = vec![];
        self.input.eat_ws()?;
        if self.input.peek_ch() == ')' {
            return Ok(args);
        }
        loop {
            self.input.eat_ws()?;
            if self.input.match_token("...")? {
                // A literal `...` only appears in the function *type*, not the
                // argument list, but tolerate it defensively.
                break;
            }
            let ty = self.parse_type()?;
            self.skip_attrs()?;
            let val = self.parse_value()?;
            args.push(TypedVal { ty, val });
            if !self.input.match_token(",")? {
                break;
            }
        }
        Ok(args)
    }

    /// Consume trailing per-instruction operands: `, align N` and metadata
    /// attachments (`, !tbaa !5`). Returns the alignment if present.
    fn parse_trailing(&mut self) -> Result<Option<u64>, ParseError>
    {
        let mut align = None;
        loop {
            self.input.eat_ws()?;
            if self.input.peek_ch() != ',' {
                break;
            }
            self.input.eat_ch(); // ','
            self.input.eat_ws()?;
            if self.input.match_keyword("align")? {
                self.input.eat_ws()?;
                align = Some(self.input.parse_int(10)? as u64);
            } else if self.input.peek_ch() == '!' {
                self.skip_metadata_ref()?;
            } else {
                return self.input.parse_error("unexpected trailing operand");
            }
        }
        Ok(align)
    }

    /// Skip a metadata attachment such as `!tbaa !5` or `!llvm.loop !{...}`.
    fn skip_metadata_ref(&mut self) -> Result<(), ParseError>
    {
        self.input.eat_ws()?;
        if !self.input.match_char('!') {
            return self.input.parse_error("expected '!'");
        }
        self.read_word(); // metadata kind (tbaa, dbg, llvm.loop, ...)
        self.input.eat_ws()?;
        if self.input.peek_ch() == '!' {
            self.input.eat_ch();
            if self.input.peek_ch() == '{' {
                self.skip_braces()?;
            } else {
                self.read_word();
            }
        }
        Ok(())
    }

    /// Skip a balanced `{ ... }` group starting at the next `{`.
    fn skip_braces(&mut self) -> Result<(), ParseError>
    {
        if !self.input.match_char('{') {
            return self.input.parse_error("expected '{'");
        }
        let mut depth = 1;
        loop {
            if self.input.eof() {
                return self.input.parse_error("unbalanced braces");
            }
            match self.input.eat_ch() {
                '{' => depth += 1,
                '}' => {
                    depth -= 1;
                    if depth == 0 {
                        break;
                    }
                }
                _ => {}
            }
        }
        Ok(())
    }

    // -----------------------------------------------------------------------
    // Terminators
    // -----------------------------------------------------------------------

    fn parse_ret(&mut self) -> Result<Terminator, ParseError>
    {
        if self.input.match_keyword("void")? {
            self.parse_trailing()?;
            return Ok(Terminator::Ret { val: None });
        }
        let ty = self.parse_type()?;
        let val = self.parse_value()?;
        self.parse_trailing()?;
        Ok(Terminator::Ret { val: Some(TypedVal { ty, val }) })
    }

    fn parse_br(&mut self) -> Result<Terminator, ParseError>
    {
        if self.input.match_keyword("label")? {
            let dest = self.parse_local_name()?;
            self.parse_trailing()?;
            return Ok(Terminator::Br { cond: None, if_true: dest, if_false: None });
        }

        let _ty = self.parse_type()?; // i1
        let cond = self.parse_value()?;
        self.input.expect_token(",")?;
        self.expect_keyword("label")?;
        let if_true = self.parse_local_name()?;
        self.input.expect_token(",")?;
        self.expect_keyword("label")?;
        let if_false = self.parse_local_name()?;
        self.parse_trailing()?;
        Ok(Terminator::Br { cond: Some(cond), if_true, if_false: Some(if_false) })
    }

    fn parse_switch(&mut self) -> Result<Terminator, ParseError>
    {
        let ty = self.parse_type()?;
        let val = self.parse_value()?;
        self.input.expect_token(",")?;
        self.expect_keyword("label")?;
        let default = self.parse_local_name()?;
        self.input.expect_token("[")?;

        let mut cases = vec![];
        loop {
            self.input.eat_ws()?;
            if self.input.peek_ch() == ']' {
                break;
            }
            let _cty = self.parse_type()?;
            let cval = self.parse_value()?;
            let case = match cval {
                Value::Int(v) => v,
                _ => return self.input.parse_error("switch case must be an integer constant"),
            };
            self.input.expect_token(",")?;
            self.expect_keyword("label")?;
            let dest = self.parse_local_name()?;
            cases.push((case, dest));
        }
        self.input.expect_token("]")?;
        self.parse_trailing()?;
        Ok(Terminator::Switch { ty, val, default, cases })
    }

    // -----------------------------------------------------------------------
    // Types
    // -----------------------------------------------------------------------

    fn parse_type(&mut self) -> Result<Type, ParseError>
    {
        self.input.eat_ws()?;

        if self.input.match_keyword("void")? {
            return Ok(Type::Void);
        }
        if self.input.match_keyword("float")? {
            return Ok(Type::Float);
        }
        if self.input.match_keyword("double")? {
            return Ok(Type::Double);
        }
        if self.input.match_keyword("ptr")? {
            if self.input.match_keyword("addrspace")? {
                self.skip_parens()?;
            }
            return Ok(Type::Ptr);
        }
        if self.input.match_keyword("label")? {
            return Ok(Type::Label);
        }
        if self.input.match_keyword("metadata")? {
            return Ok(Type::Metadata);
        }

        let c = self.input.peek_ch();

        // Integer type `iN`.
        if c == 'i' {
            self.input.eat_ch();
            let width = self.input.parse_int(10)? as u32;
            return Ok(Type::Int(width));
        }
        // Array type `[N x T]`.
        if c == '[' {
            self.input.eat_ch();
            self.input.eat_ws()?;
            let len = self.input.parse_int(10)? as u64;
            self.expect_keyword("x")?;
            let elem = self.parse_type()?;
            self.input.expect_token("]")?;
            return Ok(Type::Array { len, elem: Box::new(elem) });
        }
        // Literal struct `{ ... }`.
        if c == '{' {
            let fields = self.parse_struct_fields()?;
            return Ok(Type::Struct(StructBody { packed: false, fields }));
        }
        // Packed literal struct `<{ ... }>`.
        if c == '<' {
            self.input.eat_ch(); // '<'
            let fields = self.parse_struct_fields()?;
            self.input.expect_token(">")?;
            return Ok(Type::Struct(StructBody { packed: true, fields }));
        }
        // Named struct reference `%struct.foo`.
        if c == '%' {
            let name = self.parse_local_name()?;
            return Ok(Type::NamedStruct(name));
        }

        self.input.parse_error("expected a type")
    }

    /// Parse `{ T, T, ... }` (consumes both braces).
    fn parse_struct_fields(&mut self) -> Result<Vec<Type>, ParseError>
    {
        self.input.expect_token("{")?;
        let mut fields = vec![];
        self.input.eat_ws()?;
        if self.input.peek_ch() != '}' {
            loop {
                fields.push(self.parse_type()?);
                if !self.input.match_token(",")? {
                    break;
                }
            }
        }
        self.input.expect_token("}")?;
        Ok(fields)
    }

    // -----------------------------------------------------------------------
    // Values
    // -----------------------------------------------------------------------

    fn parse_value(&mut self) -> Result<Value, ParseError>
    {
        self.input.eat_ws()?;
        let c = self.input.peek_ch();

        if c == '%' {
            return Ok(Value::Local(self.parse_local_name()?));
        }
        if c == '@' {
            return Ok(Value::Global(self.parse_global_name()?));
        }
        // A numeric constant: integer, or floating-point (decimal or the LLVM
        // `0x`-hex bit-pattern form). Floats always carry a `.`/exponent or the
        // hex prefix, so the token shape disambiguates.
        if c == '-' || c == '+' || c.is_ascii_digit() {
            if self.input.peek_chars(&['0', 'x']) || self.input.peek_chars(&['0', 'X']) {
                return Ok(Value::Float(self.input.parse_hex_float()?));
            }
            let s = self.input.read_numeric();
            if s.is_empty() {
                return self.input.parse_error("expected a numeric constant");
            }
            if s.contains('.') || s.contains('e') || s.contains('E') {
                return match s.parse::<f64>() {
                    Ok(f) => Ok(Value::Float(f)),
                    Err(_) => self.input.parse_error("invalid floating-point constant"),
                };
            }
            return match s.parse::<i128>() {
                Ok(v) => Ok(Value::Int(v)),
                Err(_) => self.input.parse_error("invalid integer constant"),
            };
        }
        if self.input.match_keyword("true")? {
            return Ok(Value::Int(1));
        }
        if self.input.match_keyword("false")? {
            return Ok(Value::Int(0));
        }
        if self.input.match_keyword("null")? {
            return Ok(Value::Null);
        }
        if self.input.match_keyword("undef")? {
            return Ok(Value::Undef);
        }
        if self.input.match_keyword("poison")? {
            return Ok(Value::Poison);
        }
        if self.input.match_keyword("zeroinitializer")? {
            return Ok(Value::ZeroInit);
        }

        // `c"..."` byte-string constant.
        if self.input.peek_chars(&['c', '"']) {
            return Ok(Value::CStr(self.parse_cstring()?));
        }
        // `[ ... ]` array constant.
        if c == '[' {
            return self.parse_array_const();
        }
        // `{ ... }` struct constant.
        if c == '{' {
            return Ok(Value::Struct(self.parse_struct_const()?));
        }
        // `<{ ... }>` packed struct constant.
        if self.input.peek_chars(&['<', '{']) {
            self.input.eat_ch(); // '<'
            let fields = self.parse_struct_const()?;
            self.input.expect_token(">")?;
            return Ok(Value::Struct(fields));
        }

        // Constant expressions.
        if self.input.match_keyword("getelementptr")? {
            return self.parse_ce_gep();
        }
        if let Some(op) = self.match_conv_op()? {
            let (val, to) = self.parse_ce_conv_body()?;
            return Ok(Value::ConstExpr(Box::new(ConstExpr::Conv { op, val, to })));
        }
        if let Some(op) = self.match_bin_op()? {
            self.skip_arith_flags()?;
            self.input.expect_token("(")?;
            let lhs = self.parse_typed_val()?;
            self.input.expect_token(",")?;
            let rhs = self.parse_typed_val()?;
            self.input.expect_token(")")?;
            return Ok(Value::ConstExpr(Box::new(ConstExpr::Bin { op, lhs, rhs })));
        }

        self.input.parse_error("value form not yet supported")
    }

    /// If the next token is a conversion opcode, consume it and return it.
    fn match_conv_op(&mut self) -> Result<Option<ConvOp>, ParseError>
    {
        const OPS: &[(&str, ConvOp)] = &[
            ("trunc", ConvOp::Trunc),
            ("zext", ConvOp::ZExt),
            ("sext", ConvOp::SExt),
            ("ptrtoint", ConvOp::PtrToInt),
            ("inttoptr", ConvOp::IntToPtr),
            ("bitcast", ConvOp::BitCast),
        ];
        for (kw, op) in OPS {
            if self.input.match_keyword(kw)? {
                return Ok(Some(*op));
            }
        }
        Ok(None)
    }

    /// If the next token is a binary opcode, consume it and return it.
    fn match_bin_op(&mut self) -> Result<Option<BinOp>, ParseError>
    {
        const OPS: &[(&str, BinOp)] = &[
            ("add", BinOp::Add), ("sub", BinOp::Sub), ("mul", BinOp::Mul),
            ("udiv", BinOp::UDiv), ("sdiv", BinOp::SDiv),
            ("urem", BinOp::URem), ("srem", BinOp::SRem),
            ("and", BinOp::And), ("or", BinOp::Or), ("xor", BinOp::Xor),
            ("shl", BinOp::Shl), ("lshr", BinOp::LShr), ("ashr", BinOp::AShr),
        ];
        for (kw, op) in OPS {
            if self.input.match_keyword(kw)? {
                return Ok(Some(*op));
            }
        }
        Ok(None)
    }

    /// Parse a `<type> <value>` pair.
    fn parse_typed_val(&mut self) -> Result<TypedVal, ParseError>
    {
        let ty = self.parse_type()?;
        let val = self.parse_value()?;
        Ok(TypedVal { ty, val })
    }

    /// Parse the bytes of a `c"..."` string constant. LLVM only escapes via
    /// `\XX` (two hex digits); a bare `\` followed by a non-hex char (e.g.
    /// `\\`) denotes that literal character.
    fn parse_cstring(&mut self) -> Result<Vec<u8>, ParseError>
    {
        self.input.eat_ch(); // 'c'
        self.input.eat_ch(); // opening '"'
        let mut bytes = vec![];
        loop {
            if self.input.eof() {
                return self.input.parse_error("unterminated string constant");
            }
            let ch = self.input.eat_ch();
            if ch == '"' {
                break;
            }
            if ch == '\\' {
                let h0 = self.input.peek_ch();
                if let Some(d0) = h0.to_digit(16) {
                    self.input.eat_ch();
                    let d1 = match self.input.eat_ch().to_digit(16) {
                        Some(d) => d,
                        None => return self.input.parse_error("invalid hex escape"),
                    };
                    bytes.push(((d0 << 4) | d1) as u8);
                } else {
                    // `\\` and similar: the next char is taken literally.
                    bytes.push(self.input.eat_ch() as u8);
                }
            } else {
                bytes.push(ch as u8);
            }
        }
        Ok(bytes)
    }

    /// Parse `[ <ty> <val>, ... ]`.
    fn parse_array_const(&mut self) -> Result<Value, ParseError>
    {
        self.input.expect_token("[")?;
        let mut elems = vec![];
        self.input.eat_ws()?;
        if self.input.peek_ch() != ']' {
            loop {
                elems.push(self.parse_typed_val()?);
                if !self.input.match_token(",")? {
                    break;
                }
            }
        }
        self.input.expect_token("]")?;
        Ok(Value::Array(elems))
    }

    /// Parse `{ <ty> <val>, ... }` (consumes both braces).
    fn parse_struct_const(&mut self) -> Result<Vec<TypedVal>, ParseError>
    {
        self.input.expect_token("{")?;
        let mut fields = vec![];
        self.input.eat_ws()?;
        if self.input.peek_ch() != '}' {
            loop {
                fields.push(self.parse_typed_val()?);
                if !self.input.match_token(",")? {
                    break;
                }
            }
        }
        self.input.expect_token("}")?;
        Ok(fields)
    }

    /// Parse a constant `getelementptr [flags] (<base_ty>, <base>, <idx>...)`.
    fn parse_ce_gep(&mut self) -> Result<Value, ParseError>
    {
        let inbounds = self.parse_gep_flags()?;
        self.input.expect_token("(")?;
        let base_ty = self.parse_type()?;
        self.input.expect_token(",")?;
        let base = self.parse_typed_val()?;
        let mut indices = vec![];
        while self.input.match_token(",")? {
            indices.push(self.parse_typed_val()?);
        }
        self.input.expect_token(")")?;
        Ok(Value::ConstExpr(Box::new(ConstExpr::GetElementPtr {
            inbounds,
            base_ty,
            base,
            indices,
        })))
    }

    /// Parse the body of an `inttoptr`/`ptrtoint` const-expr: `(<tv> to <ty>)`.
    fn parse_ce_conv_body(&mut self) -> Result<(TypedVal, Type), ParseError>
    {
        self.input.expect_token("(")?;
        let val = self.parse_typed_val()?;
        self.expect_keyword("to")?;
        let to = self.parse_type()?;
        self.input.expect_token(")")?;
        Ok((val, to))
    }

    /// Consume getelementptr flags (`inbounds`, `nuw`, `nusw`), returning
    /// whether `inbounds` was present.
    fn parse_gep_flags(&mut self) -> Result<bool, ParseError>
    {
        let mut inbounds = false;
        loop {
            if self.input.match_keyword("inbounds")? {
                inbounds = true;
            } else if self.input.match_keyword("nuw")? || self.input.match_keyword("nusw")? {
                // flag with no effect on our model
            } else {
                break;
            }
        }
        Ok(inbounds)
    }

    // -----------------------------------------------------------------------
    // Attribute / token skipping helpers
    // -----------------------------------------------------------------------

    /// Consume and discard a run of attributes at the current position
    /// (linkage, parameter/function attributes, `#N` group refs, quoted
    /// `"key"="val"` pairs). Stops at the first token that isn't an attribute.
    fn skip_attrs(&mut self) -> Result<(), ParseError>
    {
        loop {
            self.input.eat_ws()?;
            let c = self.input.peek_ch();

            // Attribute-group reference: `#0`, `#33`.
            if c == '#' {
                self.input.eat_ch();
                while self.input.peek_ch().is_ascii_digit() {
                    self.input.eat_ch();
                }
                continue;
            }

            // Quoted attribute, possibly `"key"="value"`.
            if c == '"' {
                self.input.parse_str('"')?;
                self.input.eat_ws()?;
                if self.input.peek_ch() == '=' {
                    self.input.eat_ch();
                    self.input.eat_ws()?;
                    if self.input.peek_ch() == '"' {
                        self.input.parse_str('"')?;
                    }
                }
                continue;
            }

            // `keyword(...)` attributes.
            let mut matched = false;
            for kw in PAREN_ATTRS {
                if self.input.match_keyword(kw)? {
                    self.skip_parens()?;
                    matched = true;
                    break;
                }
            }
            if matched {
                continue;
            }

            // `align N`.
            if self.input.match_keyword("align")? {
                self.input.eat_ws()?;
                self.input.parse_int(10)?;
                continue;
            }

            // No-argument attribute keywords.
            for kw in FLAG_ATTRS {
                if self.input.match_keyword(kw)? {
                    matched = true;
                    break;
                }
            }
            if matched {
                continue;
            }

            break;
        }

        Ok(())
    }

    /// Skip a balanced parenthesized group starting at the next `(`.
    fn skip_parens(&mut self) -> Result<(), ParseError>
    {
        self.input.eat_ws()?;
        if !self.input.match_char('(') {
            return self.input.parse_error("expected '('");
        }
        let mut depth = 1;
        loop {
            if self.input.eof() {
                return self.input.parse_error("unbalanced parentheses");
            }
            match self.input.eat_ch() {
                '(' => depth += 1,
                ')' => {
                    depth -= 1;
                    if depth == 0 {
                        break;
                    }
                }
                _ => {}
            }
        }
        Ok(())
    }

    /// Consume the rest of the current line, including the newline.
    fn skip_line(&mut self)
    {
        loop {
            if self.input.eof() || self.input.eat_ch() == '\n' {
                break;
            }
        }
    }

    // -----------------------------------------------------------------------
    // Small lexical helpers
    // -----------------------------------------------------------------------

    /// Read an LLVM identifier word (no sigil): the character class allowed in
    /// unquoted names — alphanumerics plus `_ . - $`. Assumes leading
    /// whitespace has already been consumed.
    fn read_word(&mut self) -> String
    {
        let mut s = String::new();
        loop {
            let c = self.input.peek_ch();
            if c.is_ascii_alphanumeric() || c == '_' || c == '.' || c == '-' || c == '$' {
                s.push(c);
                self.input.eat_ch();
            } else {
                break;
            }
        }
        s
    }

    /// Parse `%name`, returning the bare name.
    fn parse_local_name(&mut self) -> Result<String, ParseError>
    {
        self.input.eat_ws()?;
        if !self.input.match_char('%') {
            return self.input.parse_error("expected '%' local name");
        }
        let name = self.read_word();
        if name.is_empty() {
            return self.input.parse_error("expected local name after '%'");
        }
        Ok(name)
    }

    /// Parse `@name`, returning the bare name.
    fn parse_global_name(&mut self) -> Result<String, ParseError>
    {
        self.input.eat_ws()?;
        if !self.input.match_char('@') {
            return self.input.parse_error("expected '@' global name");
        }
        let name = self.read_word();
        if name.is_empty() {
            return self.input.parse_error("expected global name after '@'");
        }
        Ok(name)
    }

    /// Parse a `"..."` string (leading whitespace allowed).
    fn parse_quoted(&mut self) -> Result<String, ParseError>
    {
        self.input.eat_ws()?;
        if self.input.peek_ch() != '"' {
            return self.input.parse_error("expected '\"'");
        }
        self.input.parse_str('"')
    }

    fn expect_keyword(&mut self, kw: &str) -> Result<(), ParseError>
    {
        if self.input.match_keyword(kw)? {
            Ok(())
        } else {
            self.input.parse_error(&format!("expected '{}'", kw))
        }
    }
}

/// Build a basic block from the pending label + instructions and the given
/// terminator, resetting the accumulators.
fn finish_block(label: &mut Option<String>, insts: &mut Vec<Inst>, term: Terminator) -> BasicBlock
{
    BasicBlock {
        label: label.take().unwrap_or_else(|| "0".to_string()),
        insts: mem::take(insts),
        term,
    }
}
