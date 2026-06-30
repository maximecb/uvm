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
    "addrspace",
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
                    blocks.push(finish_block(&mut label, &mut insts, Terminator::Unreachable));
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
        // To be filled in as test inputs grow: add, sub, ..., icmp, phi, load,
        // getelementptr, call, zext/sext/trunc, select, alloca, freeze, ...
        self.input
            .parse_error(&format!("instruction not yet supported: '{}'", op))
    }

    // -----------------------------------------------------------------------
    // Terminators
    // -----------------------------------------------------------------------

    fn parse_ret(&mut self) -> Result<Terminator, ParseError>
    {
        if self.input.match_keyword("void")? {
            return Ok(Terminator::Ret { val: None });
        }
        let ty = self.parse_type()?;
        let val = self.parse_value()?;
        Ok(Terminator::Ret { val: Some(TypedVal { ty, val }) })
    }

    fn parse_br(&mut self) -> Result<Terminator, ParseError>
    {
        if self.input.match_keyword("label")? {
            let dest = self.parse_local_name()?;
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
        if c == '-' || c.is_ascii_digit() {
            let neg = self.input.match_char('-');
            let mut v = self.input.parse_int(10)?;
            if neg {
                v = -v;
            }
            return Ok(Value::Int(v));
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

        // Aggregate constants (c"...", [...], {...}) and constant expressions
        // (getelementptr/inttoptr/ptrtoint) are added when global initializers
        // are implemented.
        self.input.parse_error("value form not yet supported")
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
