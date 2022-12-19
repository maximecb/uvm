use std::fmt;
use std::convert::{TryFrom};
use std::collections::HashMap;
use crate::vm::{VM, MemBlock, Op};

#[derive(Debug)]
pub struct ParseError
{
    msg: String,
    line_no: usize,
    col_no: usize,
}

impl ParseError
{
    fn new(input: &Input, msg: &str) -> Self
    {
        ParseError {
            msg: msg.to_string(),
            line_no: input.line_no,
            col_no: input.col_no
        }
    }
}

impl fmt::Display for ParseError
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "parse error")
    }
}

struct Input
{
    input: Vec<char>,

    idx: usize,

    line_no: usize,

    col_no: usize,
}

impl Input
{
    fn new(input: String) -> Self
    {
        Self {
            input: input.chars().collect(),
            idx: 0,
            line_no: 1,
            col_no: 1,
        }
    }

    /// Shortcut for yielding a parse error wrapped in a result type
    fn parse_error<T>(&self, msg: &str) -> Result<T, ParseError>
    {
        Err(ParseError::new(self, msg))
    }

    /// Check if we have reached the end of the input
    fn eof(&self) -> bool
    {
        self.idx >= self.input.len()
    }

    /// Peek at the next character in the input
    fn peek_ch(&self) -> char
    {
        if self.idx < self.input.len() {
            return self.input[self.idx]
        }

        '\0'
    }

    /// Consume one character from the input
    fn eat_ch(&mut self) -> char
    {
        if self.idx >= self.input.len() {
            panic!("unexpected end of input");
        }

        let ch = self.input[self.idx];
        self.idx += 1;

        if ch == '\n' {
            self.line_no += 1;
            self.col_no = 1;
        }
        else
        {
            self.col_no += 1;
        }

        ch
    }

    /// Consume whitespace characters (excluding newlines)
    fn eat_ws(&mut self)
    {
        loop
        {
            let ch = self.peek_ch();

            match ch {
                '\r' |
                '\n' |
                '\t' |
                ' ' => {
                    self.eat_ch();
                }

                _ => break
            }
        }
    }

    /// Check that a separator character is present
    fn expect_sep(&mut self) -> Result<(), ParseError>
    {
        match self.peek_ch() {
            '\r' |
            '\n' |
            '\t' |
            ' ' |
            '\0' |
            '#' |
            ';' |
            ':' => {
                Ok(())
            }

            _ => {
                self.parse_error(&format!("expected separator after token"))
            }
        }
    }

    /// Consume characters until the end of a comment
    fn eat_comment(&mut self)
    {
        loop
        {
            let ch = self.peek_ch();

            if ch == '\0' {
                break;
            }

            if ch == '\n' {
                self.eat_ch();
                break;
            }

            self.eat_ch();
        }
    }

    /// Check if the input matches a given string
    fn match_str(&mut self, token: &str) -> bool
    {
        let tok_chars: Vec<char> = token.chars().collect();
        let tok_end_idx = self.idx + tok_chars.len();

        // If the token matches the input
        if self.input[self.idx .. tok_end_idx] == tok_chars {
            for i in 0..tok_chars.len() {
                self.eat_ch();
            }

            return true;
        }

        false
    }

    /// Expect a token to be present, which can be preceded by whitespace
    fn expect_token(&mut self, token: &str) -> Result<(), ParseError>
    {
        self.eat_ws();

        if !self.match_str(token) {
            return self.parse_error(&format!("expected {}", token));
        }

        Ok(())
    }

    /// Parse a decimal integer
    fn parse_int(&mut self) -> Result<i128, ParseError>
    {
        let mut val: i128 = 0;

        let sign = if self.match_str("-") { -1 } else { 1 };

        let mut base = 10;
        if self.match_str("0x") { base = 16 };
        if self.match_str("0b") { base = 2 };

        loop
        {
            let ch = self.eat_ch();

            // There must be at least one digit
            if !ch.is_digit(base) {
                return self.parse_error("expected digit");
            }

            val = (base as i128) * val + (ch.to_digit(base).unwrap() as i128);

            let ch = self.peek_ch();

            // Allow underscores as separators
            if ch == '_' {
                self.eat_ch();
                continue;
            }

            if ch == '\0' {
                break;
            }

            if !ch.is_digit(base) {
                break;
            }
        }

        return Ok(sign * val);
    }

    /// Parse a string literal
    fn parse_str(&mut self) -> Result<String, ParseError>
    {
        let open_ch = self.eat_ch();
        assert!(open_ch == '"');

        let mut out = String::new();

        loop
        {
            if self.eof() {
                return self.parse_error("unexpected end of input while parsing string literal");
            }

            let ch = self.eat_ch();

            if ch == open_ch {
                break;
            }

            if ch == '\\' {
                match self.eat_ch() {
                    '\\' => out.push('\\'),
                    't' => out.push('\t'),
                    'n' => out.push('\n'),
                    _ => return self.parse_error("unknown escape sequence")
                }

                continue;
            }

            out.push(ch);
        }

        return Ok(out);
    }

    /// Parse an identifier
    fn parse_ident(&mut self) -> Result<String, ParseError>
    {
        let mut ident = "".to_string();

        loop
        {
            let ch = self.peek_ch();

            if ch == '\0' {
                break;
            }

            if !ch.is_ascii_alphanumeric() && ch != '_' {
                break;
            }

            ident.push(ch);
            self.eat_ch();
        }

        if ident.len() == 0 {
            return self.parse_error("expected identifier");
        }

        Ok(ident)
    }
}

#[derive(Copy, Clone, PartialEq)]
enum Section
{
    Code,
    Data,
}

#[derive(Copy, Clone)]
struct LabelDef
{
    section: Section,
    pos: usize,
    line_no: usize,
    col_no: usize,
}

#[derive(Copy, Clone)]
enum LabelRefKind
{
    Address32,
    Offset32(usize),
}

struct LabelRef
{
    name: String,
    pos: usize,
    line_no: usize,
    col_no: usize,
    kind: LabelRefKind
}

pub struct Assembler
{
    /// Map of syscall names to indices
    syscall_map: HashMap<String, u16>,

    /// Table of syscall names, sorted by index
    syscall_tbl: Vec<String>,

    code: MemBlock,

    data: MemBlock,

    /// Label definitions (name, position)
    label_defs: HashMap<String, LabelDef>,

    /// References to labels (name, position)
    label_refs: Vec<LabelRef>,

    /// Current section
    section: Section,
}

impl Assembler
{
    pub fn new() -> Self
    {
        Self {
            syscall_map: HashMap::new(),
            syscall_tbl: Vec::new(),
            code: MemBlock::new(),
            data: MemBlock::new(),
            label_defs: HashMap::default(),
            label_refs: Vec::default(),
            section: Section::Code,
        }
    }

    fn parse_input(mut self, input: &mut Input) -> Result<VM, ParseError>
    {
        // Until we've reached the end of the input
        loop
        {
            input.eat_ws();

            if input.eof() {
                break
            }

            self.parse_line(input)?;
        }

        // Link the labels
        for label_ref in self.label_refs {
            let def = self.label_defs.get(&label_ref.name);

            if def.is_none() {
                return Err(ParseError {
                    msg: format!("label not found {}", label_ref.name),
                    line_no: label_ref.line_no,
                    col_no: label_ref.col_no,
                });
            }

            let def = *def.unwrap();

            match label_ref.kind {
                LabelRefKind::Address32 => {
                    let ptr32 = u32::try_from(def.pos);

                    if ptr32.is_err() {
                        return Err(ParseError {
                            msg: format!("address doesn't fit in u32 {}", label_ref.name),
                            line_no: label_ref.line_no,
                            col_no: label_ref.col_no,
                        });
                    }

                    self.code.write(label_ref.pos, ptr32.unwrap());
                }

                LabelRefKind::Offset32(end_offset) => {
                    assert!(def.section == Section::Code);
                    let offs32 = (def.pos as i32) - ((label_ref.pos + end_offset) as i32 + 4);
                    self.code.write(label_ref.pos, offs32);
                }
            }
        }

        Ok(VM::new(self.code, self.data, self.syscall_tbl))
    }

    pub fn parse_file(mut self, file_name: &str) -> Result<VM, ParseError>
    {
        let input_str = std::fs::read_to_string(file_name).unwrap();
        let mut input = Input::new(input_str);
        return self.parse_input(&mut input);
    }

    /// Parse a string of source code
    pub fn parse_str(mut self, src: &str) -> Result<VM, ParseError>
    {
        let mut input = Input::new(src.to_string());
        return self.parse_input(&mut input);
    }

    /// Parse an integer argument
    fn parse_int_arg<T>(&self, input: &mut Input) -> Result<T, ParseError> where T: TryFrom<i128>
    {
        input.eat_ws();

        let ch = input.peek_ch();

        // If this is an integer literal
        if ch.is_digit(10) || ch == '-' {
            let int_val = input.parse_int()?;

            return match int_val.try_into() {
                Ok(out_val) => Ok(out_val),
                Err(_) => input.parse_error("integer literal did not fit required size")
            }
        }

        input.parse_error("expected integer argument")
    }

    /// Get the memory block for the current section
    fn mem(&mut self) -> &mut MemBlock
    {
        match self.section {
            Section::Code => &mut self.code,
            Section::Data => &mut self.data,
        }
    }

    /// Add a new label reference at the current position
    fn add_label_ref(&mut self, input: &Input, name: String, kind: LabelRefKind)
    {
        assert!(self.section == Section::Code);

        self.label_refs.push(
            LabelRef{
                name: name,
                pos: self.code.len(),
                line_no: input.line_no,
                col_no: input.line_no,
                kind: kind
            }
        );

        match kind {
            LabelRefKind::Address32 => self.code.push_u32(0),
            LabelRefKind::Offset32(_) => self.code.push_u32(0),
        }
    }

    /// Parse the current line of the input
    fn parse_line(&mut self, input: &mut Input) -> Result<(), ParseError>
    {
        let ch = input.peek_ch();

        // If this line is empty
        if ch == '\n' {
            input.eat_ch();
            return Ok(());
        }

        // If this is a comment
        if ch == '#' {
            input.eat_comment();
            return Ok(());
        }

        // If this is an assembler command
        if ch == '.' {
            input.eat_ch();
            let cmd = input.parse_ident()?;

            input.expect_sep()?;
            input.eat_ws();

            self.parse_cmd(input, cmd)?;

            return Ok(());
        }

        // If this is the start of an identifier
        if ch.is_ascii_alphabetic() || ch == '_' {
            let ident = input.parse_ident()?;

            input.expect_sep()?;
            input.eat_ws();

            if input.match_str(":") {
                if self.label_defs.get(&ident).is_some() {
                    return input.parse_error(&format!("label already defined {}", ident));
                }

                let label_pos = self.mem().len();
                self.label_defs.insert(
                    ident,
                    LabelDef {
                        section: self.section,
                        pos: label_pos,
                        line_no: input.line_no,
                        col_no: input.col_no,
                    }
                );
            }
            else if self.section == Section::Code
            {
                self.parse_insn(input, ident)?;
            }

            return Ok(());
        }

        input.parse_error("invalid input")
    }

    /// Parse an assembler command
    fn parse_cmd(&mut self, input: &mut Input, cmd: String) -> Result<(), ParseError>
    {
        match cmd.as_str() {
            "code" => self.section = Section::Code,
            "data" => self.section = Section::Data,

            "zero" => {
                let num_bytes: u32 = self.parse_int_arg(input)?;
                let mem = self.mem();
                for i in 0..num_bytes {
                    mem.push_u8(0);
                }
            }

            "fill" => {
                let num_bytes: u32 = self.parse_int_arg(input)?;

                input.expect_token(",")?;

                let val: u8 = self.parse_int_arg(input)?;
                let mem = self.mem();
                for i in 0..num_bytes {
                    mem.push_u8(val);
                }
            }

            // Unsigned 8-bit integer constant
            "u8" => {
                let val: u8 = self.parse_int_arg(input)?;
                self.mem().push_u8(val);
            }

            // Unsigned 64-bit integer constant
            "u64" => {
                let val: u64 = self.parse_int_arg(input)?;
                self.mem().push_u64(val);
            }

            // Null-terminated UTF-8 string
            "stringz" => {
                let val = input.parse_str()?;

                let mem = self.mem();
                for byte in val.bytes() {
                    mem.push_u8(byte);
                }

                // Write a null terminator byte
                mem.push_u8(0);
            }

            _ => {
                return input.parse_error(&format!("unknown assembler command \"{}\"", cmd))
            }
        }

        input.expect_token(";")?;

        Ok(())
    }

    /// Parse an instruction and its arguments
    fn parse_insn(&mut self, input: &mut Input, op_name: String) -> Result<(), ParseError>
    {
        match op_name.as_str() {
            "nop" => self.code.push_op(Op::nop),

            "pop" => self.code.push_op(Op::pop),
            "dup" => self.code.push_op(Op::dup),
            "swap" => self.code.push_op(Op::swap),

            "popn" => {
                let n: u8 = self.parse_int_arg(input)?;
                self.code.push_op(Op::popn);
                self.code.push_u8(n);
            }

            "get_arg" => {
                let idx: u8 = self.parse_int_arg(input)?;
                self.code.push_op(Op::get_arg);
                self.code.push_u8(idx);
            }

            "get_local" => {
                let idx: u8 = self.parse_int_arg(input)?;
                self.code.push_op(Op::get_local);
                self.code.push_u8(idx);
            }

            "set_local" => {
                let idx: u8 = self.parse_int_arg(input)?;
                self.code.push_op(Op::set_local);
                self.code.push_u8(idx);
            }

            "push_i8" => {
                let val: i8 = self.parse_int_arg(input)?;
                self.code.push_op(Op::push_i8);
                self.code.push_i8(val);
            }

            "push_u32" => {
                let val: u32 = self.parse_int_arg(input)?;
                self.code.push_op(Op::push_u32);
                self.code.push_u32(val);
            }

            "push_u64" => {
                let val: u64 = self.parse_int_arg(input)?;
                self.code.push_op(Op::push_u64);
                self.code.push_u64(val);
            }

            // Push a pointer to a label
            "push_p32" => {
                let label_name = input.parse_ident()?;
                self.code.push_op(Op::push_u32);
                self.add_label_ref(input, label_name, LabelRefKind::Address32);
            }

            "and_i64" => self.code.push_op(Op::and_i64),
            "or_i64" => self.code.push_op(Op::or_i64),
            "xor_i64" => self.code.push_op(Op::xor_i64),

            "add_i64" => self.code.push_op(Op::add_i64),
            "sub_i64" => self.code.push_op(Op::sub_i64),
            "mul_i64" => self.code.push_op(Op::mul_i64),
            "div_i64" => self.code.push_op(Op::div_i64),
            "mod_i64" => self.code.push_op(Op::mod_i64),

            "eq_i64" => self.code.push_op(Op::eq_i64),
            "lt_i64" => self.code.push_op(Op::lt_i64),
            "le_i64" => self.code.push_op(Op::le_i64),

            "load_u8" => self.code.push_op(Op::load_u8),
            "load_u64" => self.code.push_op(Op::load_u64),
            "store_u8" => self.code.push_op(Op::store_u8),
            "store_u64" => self.code.push_op(Op::store_u64),

            "jmp" => {
                self.code.push_op(Op::jmp);
                let label_name = input.parse_ident()?;
                self.add_label_ref(input, label_name, LabelRefKind::Offset32(0));
            }

            "jz" => {
                self.code.push_op(Op::jz);
                let label_name = input.parse_ident()?;
                self.add_label_ref(input, label_name, LabelRefKind::Offset32(0));
            }

            "jnz" => {
                self.code.push_op(Op::jnz);
                let label_name = input.parse_ident()?;
                self.add_label_ref(input, label_name, LabelRefKind::Offset32(0));
            }

            "syscall" => {
                let syscall_name = input.parse_ident()?;

                // Get the index for this syscall method name
                if self.syscall_map.get(&syscall_name).is_none() {
                    let syscall_idx = self.syscall_map.len();
                    self.syscall_map.insert(syscall_name.clone(), syscall_idx.try_into().unwrap());
                    self.syscall_tbl.push(syscall_name.clone());
                }
                let syscall_idx = *self.syscall_map.get(&syscall_name).unwrap();

                self.code.push_op(Op::syscall);
                self.code.push_u16(syscall_idx);
            }

            "call" => {
                let label_name = input.parse_ident()?;
                input.expect_token(",")?;
                let argc: u8 = self.parse_int_arg(input)?;

                self.code.push_op(Op::call);
                self.add_label_ref(input, label_name, LabelRefKind::Offset32(1));
                self.code.push_u8(argc);
            }

            "ret" => self.code.push_op(Op::ret),

            "wait" => self.code.push_op(Op::wait),
            "exit" => self.code.push_op(Op::exit),

            _ => {
                return input.parse_error(&format!("unknown instruction opcode \"{}\"", op_name))
            }
        }

        input.expect_token(";")?;

        Ok(())
    }
}

#[cfg(test)]
mod tests
{
    use super::*;

    fn parse_ok(src: &str)
    {
        dbg!(src);
        let asm = Assembler::new();
        asm.parse_str(src).unwrap();
    }

    fn parse_fails(src: &str)
    {
        dbg!(src);
        let asm = Assembler::new();
        assert!(asm.parse_str(src).is_err());
    }

    fn parse_file(file_name: &str)
    {
        dbg!(file_name);
        let asm = Assembler::new();
        assert!(asm.parse_file(file_name).is_ok());
    }

    #[test]
    fn test_basics()
    {
        // Basics
        parse_ok("");
        parse_ok("# comment");
        parse_ok("push_i8  0 ; # comment");
        parse_ok(".code;\npush_u32 0xFFFFFFFF;");
        parse_ok(".code; push_u32 1_000_000;");
        parse_ok(".code; push_i8 55; push_i8 -1;");
    }

    #[test]
    fn test_labels()
    {
        // Labels
        parse_ok("FOO: push_i8 55; push_i8 55; eq_i64; jnz FOO;");
        parse_ok("FOO: push_i8 55; push_i8 55; eq_i64; jz FOO;");
        parse_ok(" FOO_BAR:   jmp  FOO_BAR; ");

        // Callback label
        parse_ok("CB: ret; push_p32 CB; exit;");
    }

    #[test]
    fn test_data()
    {
        // Data section
        parse_ok(".code;");
        parse_ok(".code; .data;");
        parse_ok(".data; .u8 255;");
        parse_ok(".data; .u64 7777;");
        parse_ok(".data; DATA_LABEL: .zero 256;");
        parse_ok(" .data;   .fill 256   ,   0xFF ;   #comment");
        parse_ok(" .data; .fill 256,0xFF;");
        parse_ok(".data; .zero 512; .code; push_u32 0xFFFF; push_i8 7; add_i64;");
        parse_ok(" .data; #comment .fill 256, 0xFF; .code; push_u64 777; #comment");
        parse_ok(".data; DATA_LABEL: .fill 256, 0xFF; .code; push_p32 DATA_LABEL;");
        parse_ok(".data; STR_LABEL: .stringz \"hi!\"; .code; push_p32 STR_LABEL;");
    }

    #[test]
    fn test_invalid()
    {
        // Failing parses
        parse_fails("1");
        parse_fails(";");
        parse_fails(".fill 256 ,, 0xFF;");
        parse_fails(".code.zero 512");
        parse_fails(".data .u640");
        parse_fails(". code");
        parse_fails("FOO: FOO: jmp FOO;");
        parse_fails("FOO: jmp BAR;");
        parse_fails("push_i8 555");
        parse_fails("push_i855;");
        parse_fails("push_i8 55; comment without hash");
    }

    #[test]
    fn parse_files()
    {
        parse_file("examples/factorial.asm");
        parse_file("examples/loop.asm");
        parse_file("examples/gradient.asm");
        parse_file("examples/circle.asm");
    }
}
