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
    fn eat_ws(&mut self) -> bool
    {
        let mut num_ws = 0;

        loop
        {
            let ch = self.peek_ch();

            match ch {
                '\r' |
                '\n' |
                '\t' |
                ' ' => {
                    self.eat_ch();
                    num_ws += 1;
                }

                _ => break
            }
        }

        return num_ws != 0
    }

    /// Check that a separator is present
    fn sep_present(&mut self) -> Result<(), ParseError>
    {
        match self.peek_ch() {
            '\r' |
            '\n' |
            '\t' |
            ' ' |
            '\0' |
            '#' |
            ';' => {
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

    fn expect_str(&mut self, token: &str)
    {
        if !self.match_str(token) {
            panic!("expected {}", token);
        }
    }

    /// Parse a decimal integer
    fn parse_int(&mut self) -> Result<i128, ParseError>
    {
        let mut val: i128 = 0;

        let sign = if self.match_str("-") { -1 } else { 1 };
        let base = if self.match_str("0x") { 16 } else { 10 };

        loop
        {
            let ch = self.eat_ch();

            // There must be at least one digit
            if !ch.is_digit(base) {
                panic!("expected digit");
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

    /// Parse an identifier
    fn parse_ident(&mut self) -> String
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

        ident
    }
}

enum LabelRefKind
{
    Data32,
    Offset32,
}

struct LabelRef
{
    name: String,
    pos: usize,
    kind: LabelRefKind
}

#[derive(PartialEq)]
enum Section
{
    Code,
    Data,
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
    label_defs: HashMap<String, usize>,

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

    fn parse_input(mut self, input: &mut Input) -> VM
    {
        // Until we've reached the end of the input
        loop
        {
            input.eat_ws();

            if input.eof() {
                break
            }

            self.parse_line(input);
        }

        // Link the labels
        for label_ref in self.label_refs {
            let def_pos = self.label_defs.get(&label_ref.name);

            if def_pos.is_none() {
                panic!("label not found {}", label_ref.name);
            }

            let def_pos = *def_pos.unwrap();

            match label_ref.kind {
                LabelRefKind::Data32 => {
                    todo!();
                }
                LabelRefKind::Offset32 => {
                    let offs32 = (def_pos as i32) - (label_ref.pos as i32 + 4);
                    self.code.write_i32(label_ref.pos, offs32);
                }
            }
        }

        VM::new(self.code, self.data, self.syscall_tbl)
    }

    pub fn parse_file(mut self, file_name: &str) -> VM
    {
        let input_str = std::fs::read_to_string(file_name).unwrap();
        let mut input = Input::new(input_str);
        return self.parse_input(&mut input);
    }

    pub fn parse_str(mut self, src: &str) -> VM
    {
        let mut input = Input::new(src.to_string());
        return self.parse_input(&mut input);
    }

    fn parse_line(&mut self, input: &mut Input)
    {
        let ch = input.peek_ch();

        // If this line is empty
        if ch == '\n' {
            input.eat_ch();
            return;
        }

        // If this is a comment
        if ch == '#' || ch == ';' {
            input.eat_comment();
            return;
        }

        // If this is an assembler command
        if ch == '.' {
            input.eat_ch();
            let cmd = input.parse_ident();

            if cmd == "" {
                panic!("expected assembler command");
            }

            if !input.eat_ws() {
                panic!("expected whitespace after .{} command", cmd);
            }

            self.parse_cmd(input, cmd);
            return;
        }

        // If this is the start of an identifier
        if ch.is_ascii_alphanumeric() || ch == '_' {
            let ident = input.parse_ident();

            let ws_present = input.eat_ws();
            let ch = input.peek_ch();

            // We can't have an identifier directly followed by an integer
            // or another unexpected token with no separation
            if !ws_present && ch != ';' && ch != ':' {
                panic!("unexpected token");
            }

            if input.match_str(":") {
                if self.label_defs.get(&ident).is_some() {
                    panic!("label already defined {}", ident)
                }

                self.label_defs.insert(ident, self.code.len());
            }
            else if self.section == Section::Code
            {
                self.parse_insn(input, ident);
            }

            return;
        }

        panic!("invalid input at {}:{}", input.line_no, input.col_no);
    }

    /// Parse an integer argument
    fn parse_int_arg<T>(&self, input: &mut Input) -> T where T: TryFrom<i128>
    {
        input.eat_ws();

        let int_val = input.parse_int().unwrap();

        match int_val.try_into() {
            Ok(out_val) => return out_val,
            Err(_) => panic!("integer literal did not fit required size")
        }
    }

    /// Get the memory block for the current section
    fn mem(&mut self) -> &mut MemBlock
    {
        match self.section {
            Section::Code => &mut self.code,
            Section::Data => &mut self.data,
        }
    }

    /// Parse an assembler command
    fn parse_cmd(&mut self, input: &mut Input, cmd: String)
    {
        match cmd.as_str() {
            "code" => self.section = Section::Code,
            "data" => self.section = Section::Data,

            "zero" => {
                let num_bytes: u32 = self.parse_int_arg(input);
                let mem = self.mem();
                for i in 0..num_bytes {
                    mem.push_u8(0);
                }
            }

            "fill" => {
                let num_bytes: u32 = self.parse_int_arg(input);
                input.eat_ws();
                input.expect_str(",");
                input.eat_ws();
                let val: u8 = self.parse_int_arg(input);
                let mem = self.mem();
                for i in 0..num_bytes {
                    mem.push_u8(val);
                }
            }

            _ => panic!("unknown assembler command \"{}\"", cmd)
        }
    }

    /// Parse an instruction and its arguments
    fn parse_insn(&mut self, input: &mut Input, op_name: String)
    {
        match op_name.as_str() {
            "push_i8" => {
                let val: i8 = self.parse_int_arg(input);
                self.code.push_op(Op::push_i8);
                self.code.push_i8(val);
            }

            "push_u32" => {
                let val: u32 = self.parse_int_arg(input);
                self.code.push_op(Op::push_u32);
                self.code.push_u32(val);
            }

            "push_u64" => {
                let val: u64 = self.parse_int_arg(input);
                self.code.push_op(Op::push_u64);
                self.code.push_u64(val);
            }

            "add_i64" => self.code.push_op(Op::add_i64),
            "sub_i64" => self.code.push_op(Op::sub_i64),
            "mul_i64" => self.code.push_op(Op::mul_i64),

            "jmp" => {
                self.code.push_op(Op::jmp);
                let label_name = input.parse_ident();
                self.label_refs.push(LabelRef{
                    name: label_name,
                    pos: self.code.len(),
                    kind: LabelRefKind::Offset32
                });
                self.code.push_i32(0);
            }

            "jnz" => {
                self.code.push_op(Op::jnz);
                let label_name = input.parse_ident();
                self.label_refs.push(LabelRef{
                    name: label_name,
                    pos: self.code.len(),
                    kind: LabelRefKind::Offset32
                });
                self.code.push_i32(0);
            }

            "jne" => {
                self.code.push_op(Op::jne);
                let label_name = input.parse_ident();
                self.label_refs.push(LabelRef{
                    name: label_name,
                    pos: self.code.len(),
                    kind: LabelRefKind::Offset32
                });
                self.code.push_i32(0);
            }

            "syscall" => {
                let syscall_name = input.parse_ident();

                if self.syscall_map.get(&syscall_name).is_none() {
                    let syscall_idx = self.syscall_map.len();
                    self.syscall_map.insert(syscall_name.clone(), syscall_idx.try_into().unwrap());
                    self.syscall_tbl.push(syscall_name.clone());
                }

                let syscall_idx = *self.syscall_map.get(&syscall_name).unwrap();

                self.code.push_op(Op::syscall);
                self.code.push_u16(syscall_idx);
            }

            "exit" => self.code.push_op(Op::exit),

            _ => panic!("unknown instruction opcode \"{}\"", op_name)
        }

        input.eat_ws();
        input.expect_str(";");

        // Whatever follows a semicolon is a comment
        input.eat_comment();
    }
}

#[cfg(test)]
mod tests
{
    use super::*;

    fn test_parses(src: &str)
    {
        let asm = Assembler::new();
        asm.parse_str(src);
    }

    #[test]
    fn test_insns()
    {
        //test_parses(".code");
        //test_parses(".code .data");

        test_parses(".code push_u32 1_000_000;");
        test_parses(".code push_i8 55; push_i8 -1;");
        test_parses("FOO: push_i8 55; push_i8 55; jne FOO;");
        test_parses(".data .zero 512 .code push_u32 0xFFFF; .push_i8 7; .add_i64;");
        test_parses(" .data #comment .fill 256, 0xFF .code push_u64 777; #comment");




    }
}
