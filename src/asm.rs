use crate::vm::{VM, MemBlock, Op};

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
        assert!(self.idx < self.input.len());

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

    /// Consume whitespace characters
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

    /// Consume characters until the end of a comment
    fn eat_comment(&mut self)
    {
        loop
        {
            let ch = self.peek_ch();

            if ch == '\n' || ch == '\0' {
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

    /// Parse a decimal integer
    fn parse_int(&mut self) -> i64
    {
        let mut val: i64 = 0;

        loop
        {
            let ch = self.peek_ch();

            if ch == '\0' {
                break;
            }

            if !ch.is_numeric() {
                break;
            }

            val = (10 * val) + (ch.to_digit(10).unwrap() as i64);

            self.eat_ch();
        }

        val
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

            if !ch.is_alphanumeric() && ch != '_' {
                break;
            }

            ident.push(ch);
            self.eat_ch();
        }

        ident
    }
}

pub struct Assembler
{
    code: MemBlock,
    data: MemBlock,
}

impl Assembler
{
    pub fn new() -> Self
    {
        Self {
            code: MemBlock::new(),
            data: MemBlock::new(),
        }
    }

    pub fn parse_file(mut self, file_name: &str) -> MemBlock
    {
        let input_str = std::fs::read_to_string(file_name).unwrap();
        let mut input = Input::new(input_str);

        // Until we've reached the end of the input
        loop
        {
            println!("eat_ws");
            input.eat_ws();

            if input.eof() {
                break
            }

            println!("parsing line");
            self.parse_line(&mut input);
        }

        self.code
    }

    fn parse_line(&mut self, input: &mut Input)
    {
        let ch = input.peek_ch();

        println!("{}", ch as u32);

        // If this is a command
        if ch == '.' {

            // TODO: handle .data and .text to switch modes


        }

        // If this is the start of an identifier
        if ch.is_alphanumeric() || ch == '_' {
            println!("parsing ident");
            let ident = input.parse_ident();
            input.eat_ws();

            println!("ident: {}", ident);


        }

        panic!("invalid input at {}:{}", input.line_no, input.col_no);
    }
}
