use std::fs;
use std::fmt;

#[derive(Debug, Copy, Clone)]
pub struct SrcPos
{
    line_no: u32,
    col_no: u32,
}

#[derive(Debug, Clone)]
pub struct ParseError
{
    pub msg: String,
    pub src_name: String,
    pub line_no: u32,
    pub col_no: u32,
}

impl ParseError
{
    pub fn new(input: &Input, msg: &str) -> Self
    {
        ParseError {
            msg: msg.to_string(),
            src_name: input.src_name.clone(),
            line_no: input.line_no,
            col_no: input.col_no
        }
    }

    /// Parse error with just an error message, no location
    pub fn msg_only<T>(msg: &str) -> Result<T, ParseError>
    {
        Err(ParseError {
            msg: msg.to_string(),
            src_name: String::new(),
            line_no: 0,
            col_no: 0,
        })
    }
}

impl fmt::Display for ParseError
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "parse error")
    }
}

/// Check if a character can be part of an identifier
pub fn is_ident_ch(ch: char) -> bool
{
    ch.is_ascii_alphanumeric() || ch == '_'
}

#[derive(Debug, Clone)]
pub struct Input
{
    // Input string to be parsed
    input: Vec<char>,

    // Current index in the input string
    idx: usize,

    // Input source name
    pub src_name: String,

    // Current line number
    pub line_no: u32,

    // Current column number
    pub col_no: u32,
}

impl Input
{
    pub fn from_file(file_name: &str) -> Self
    {
        let data = fs::read_to_string(file_name)
            .expect(&format!("could not read input file {}", file_name));
        Input::new(&data, file_name)
    }

    pub fn new(input_str: &str, src_name: &str) -> Self
    {
        Input {
            input: input_str.chars().collect(),
            src_name: src_name.to_string(),
            idx: 0,
            line_no: 1,
            col_no: 1
        }
    }

    /// Test if the end of the input has been reached
    pub fn eof(&self) -> bool
    {
        return self.idx >= self.input.len();
    }

    /// Peek at a character from the input
    pub fn peek_ch(&self) -> char
    {
        if self.idx >= self.input.len()
        {
            return '\0';
        }

        return self.input[self.idx];
    }

    /// Consume a character from the input
    pub fn eat_ch(&mut self) -> char
    {
        let ch = self.peek_ch();

        // Move to the next char
        self.idx += 1;

        if ch == '\n'
        {
            self.line_no += 1;
            self.col_no = 1;
        }
        else
        {
            self.col_no += 1;
        }

        return ch;
    }

    /// Match a single character in the input, no preceding whitespace allowed
    pub fn match_char(&mut self, ch: char) -> bool
    {
        if self.peek_ch() == ch {
            self.eat_ch();
            return true;
        }

        return false;
    }

    /// Match characters in the input, no preceding whitespace allowed
    pub fn match_chars(&mut self, chars: &[char]) -> bool
    {
        let end_pos = self.idx + chars.len();

        if end_pos > self.input.len() {
            return false;
        }

        // Compare the characters to match
        for i in 0..chars.len() {
            if chars[i] != self.input[self.idx + i] {
                return false;
            }
        }

        // Consumed the matched characters
        for i in 0..chars.len() {
            self.eat_ch();
        }

        return true;
    }

    /// Consume characters until the end of a single-line comment
    pub fn eat_comment(&mut self)
    {
        loop
        {
            // If we are at the end of the input, stop
            if self.eof() || self.eat_ch() == '\n' {
                break;
            }
        }
    }

    /// Consume characters until the end of a multi-line comment
    pub fn eat_multi_comment(&mut self) -> Result<(), ParseError>
    {
        let mut depth = 1;

        loop
        {
            if self.eof() {
                return self.parse_error(&format!("unexpected end of input inside multi-line comment"));
            }
            else if self.match_chars(&['/', '*']) {
                depth += 1;
            }
            else if self.match_chars(&['*', '/']) {
                depth -= 1;

                if depth == 0 {
                    break
                }
            }
            else
            {
                self.eat_ch();
            }
        }

        Ok(())
    }

    /// Consume whitespace
    pub fn eat_ws(&mut self) -> Result<(), ParseError>
    {
        // Until the end of the whitespace
        loop
        {
            // If we are at the end of the input, stop
            if self.eof()
            {
                break;
            }

            // Single-line comment
            if self.match_chars(&['/', '/'])
            {
                self.eat_comment();
                continue;
            }

            // Multi-line comment
            if self.match_chars(&['/', '*'])
            {
                self.eat_multi_comment()?;
                continue;
            }

            let ch = self.peek_ch();

            // Consume whitespace characters
            if ch.is_ascii_whitespace()
            {
                self.eat_ch();
                continue;
            }

            // This isn't whitespace, stop
            break;
        }

        Ok(())
    }

    /// Match a string in the input, ignoring preceding whitespace
    /// Do not use this method to match a keyword which could be
    /// an identifier.
    pub fn match_token(&mut self, token: &str) -> Result<bool, ParseError>
    {
        // Consume preceding whitespace
        self.eat_ws()?;

        let token_chars: Vec<char> = token.chars().collect();
        return Ok(self.match_chars(&token_chars));
    }

    /// Match a keyword in the input, ignoring preceding whitespace
    /// This is different from match_token because there can't be a
    /// match if the following chars are also valid identifier chars.
    pub fn match_keyword(&mut self, keyword: &str) -> Result<bool, ParseError>
    {
        self.eat_ws()?;

        let chars: Vec<char> = keyword.chars().collect();
        let end_pos = self.idx + chars.len();

        // We can't match as a keyword if the next chars are
        // valid identifier characters
        if end_pos < self.input.len() && is_ident_ch(self.input[end_pos]) {
            return Ok(false);
        }

        return Ok(self.match_chars(&chars));
    }

    /// Shortcut for yielding a parse error wrapped in a result type
    pub fn parse_error<T>(&self, msg: &str) -> Result<T, ParseError>
    {
        Err(ParseError::new(self, msg))
    }

    /// Produce an error if the input doesn't match a given token
    pub fn expect_token(&mut self, token: &str) -> Result<(), ParseError>
    {
        if self.match_token(token)? {
            return Ok(())
        }

        self.parse_error(&format!("expected token \"{}\"", token))
    }

    /// Parse a decimal integer value
    pub fn parse_int(&mut self, radix: u32) -> Result<i128, ParseError>
    {
        let mut int_val: i128 = 0;

        if self.eof() || self.peek_ch().to_digit(radix).is_none() {
            return self.parse_error("expected digit");
        }

        loop
        {
            if self.eof() {
                break;
            }

            let ch = self.peek_ch();

            // Allow underscores as separators
            if ch == '_' {
                self.eat_ch();
                continue;
            }

            let digit = ch.to_digit(radix);

            if digit.is_none() {
                break
            }

            int_val = (radix as i128) * int_val + digit.unwrap() as i128;
            self.eat_ch();
        }

        return Ok(int_val);
    }

    /// Read the characters of a numeric value into a string
    pub fn read_numeric(&mut self) -> String
    {
        fn read_digits(input: &mut Input)
        {
            let ch = input.peek_ch();

            // The first char must be a digit
            if !ch.is_ascii_digit() {
                return;
            }

            loop
            {
                let ch = input.peek_ch();
                if !ch.is_ascii_digit() && ch != '_' {
                    break;
                }
                input.eat_ch();
            }
        }

        fn read_sign(input: &mut Input)
        {
            let _ = input.match_char('+') || input.match_char('-');
        }

        let start_idx = self.idx;

        // Read optional sign
        read_sign(self);

        // Read decimal part
        read_digits(self);

        // Fractional part
        if self.match_char('.') {
            read_digits(self);
        }

        // Exponent
        if self.match_char('e') || self.match_char('E') {
            read_sign(self);
            read_digits(self);
        }

        let end_idx = self.idx;
        let num_str: String = self.input[start_idx..end_idx].iter().collect();

        // Remove any underscore separators
        let num_str = num_str.replace("_", "");

        return num_str;
    }

    /// Parse a string literal
    pub fn parse_str(&mut self, end_ch: char) -> Result<String, ParseError>
    {
        self.eat_ch();

        let mut out = String::new();

        loop
        {
            if self.eof() {
                return self.parse_error("unexpected end of input while parsing string literal");
            }

            let ch = self.eat_ch();

            if ch == end_ch {
                break;
            }

            if ch == '\\' {
                match self.eat_ch() {
                    '\\' => out.push('\\'),
                    '\'' => out.push('\''),
                    '\"' => out.push('\"'),
                    't' => out.push('\t'),
                    'r' => out.push('\r'),
                    'n' => out.push('\n'),
                    '0' => out.push('\0'),
                    _ => return self.parse_error("unknown escape sequence")
                }

                continue;
            }

            out.push(ch);
        }

        return Ok(out);
    }

    /// Parse a C-style alphanumeric identifier
    pub fn parse_ident(&mut self) -> Result<String, ParseError>
    {
        let mut ident = String::new();

        if self.eof() || !is_ident_ch(self.peek_ch()) {
            return self.parse_error("expected identifier");
        }

        loop
        {
            if self.eof() {
                break;
            }

            let ch = self.peek_ch();

            if !is_ident_ch(ch) {
                break;
            }

            ident.push(ch);
            self.eat_ch();
        }

        return Ok(ident);
    }

    /// Try to parse something using a parsing function,
    /// and backtrack if the parsing fails
    pub fn with_backtracking<T, F>(&mut self, parse_fn: F) -> Result<T, ParseError>
    where F : FnOnce(&mut Input) -> Result<T, ParseError>
    {
        let pos = self.idx;
        let line_no = self.line_no;
        let col_no = self.col_no;

        // Try to parse using the parsing function provided
        let ret = parse_fn(self);

        if ret.is_err() {
            // Backtrack
            self.idx = pos;
            self.line_no = line_no;
            self.col_no = col_no;
        }

        ret
    }

    /// Try to parse something using a parsing function,
    /// and collect the parsed input into a string representing,
    /// all the characters that were read
    pub fn collect<T, F>(&mut self, parse_fn: F) -> Result<String, ParseError>
    where F : FnOnce(&mut Input) -> Result<T, ParseError>
    {
        let pre_pos = self.idx;

        // Try to parse using the parsing function provided
        let ret = parse_fn(self);

        match ret {
            Ok(v) => {
                let post_pos = self.idx;
                let chars = &self.input[pre_pos..post_pos];
                let slice_str: String = chars.iter().collect();
                Ok(slice_str)
            }
            Err(e) => {
                Err(e)
            }
        }
    }
}
