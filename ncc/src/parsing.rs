use std::fmt;

#[derive(Debug)]
pub struct ParseError
{
    msg: String,
    line_no: u32,
    col_no: u32,
}

impl ParseError
{
    pub fn new(input: &Input, msg: &str) -> Self
    {
        ParseError {
            msg: msg.to_string(),
            line_no: input.line_no,
            col_no: input.col_no
        }
    }

    /// Parse error with just an error message, no location
    pub fn msg_only<T>(msg: &str) -> Result<T, ParseError>
    {
        Err(ParseError {
            msg: msg.to_string(),
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
    input_str: Vec<char>,

    // Input source name
    src_name: String,

    // Current position in the input string
    pos: usize,

    // Current line number
    line_no: u32,

    // Current column number
    col_no: u32,
}

impl Input
{
    pub fn new(input_str: &str, src_name: &str) -> Self
    {
        Input {
            input_str: input_str.chars().collect(),
            src_name: src_name.to_string(),
            pos: 0,
            line_no: 1,
            col_no: 1
        }
    }

    /// Test if the end of the input has been reached
    pub fn eof(&self) -> bool
    {
        return self.pos >= self.input_str.len();
    }

    /// Peek at a character from the input
    pub fn peek_ch(&self) -> char
    {
        if self.pos >= self.input_str.len()
        {
            return '\0';
        }

        return self.input_str[self.pos];
    }

    /// Consume a character from the input
    pub fn eat_ch(&mut self) -> char
    {
        let ch = self.peek_ch();

        // Move to the next char
        self.pos += 1;

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

    /// Match characters in the input, no preceding whitespace allowed
    pub fn match_chars(&mut self, chars: &[char]) -> bool
    {
        let end_pos = self.pos + chars.len();

        if end_pos > self.input_str.len() {
            return false;
        }

        // Compare the characters to match
        for i in 0..chars.len() {
            if chars[i] != self.input_str[self.pos + i] {
                return false;
            }
        }

        // Consumed the matched characters
        for i in 0..chars.len() {
            self.eat_ch();
        }

        return true;
    }

    /// Consume characters until the end of a multi-line comment
    fn eat_multi_comment(&mut self) -> Result<(), ParseError>
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
                loop
                {
                    // If we are at the end of the input, stop
                    if self.eof() || self.eat_ch() == '\n' {
                        break;
                    }
                }
            }

            // Multi-line comment
            if self.match_chars(&['/', '*'])
            {
                self.eat_multi_comment()?;
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
        let end_pos = self.pos + chars.len();

        // We can't match as a keyword if the next chars are
        // valid identifier characters
        if end_pos < self.input_str.len() && is_ident_ch(self.input_str[end_pos]) {
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

    /// Parse a string literal
    pub fn parse_str(&mut self) -> Result<String, ParseError>
    {
        let open_ch = self.eat_ch();
        assert!(open_ch == '\'' || open_ch == '"');

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
        let pos = self.pos;
        let line_no = self.line_no;
        let col_no = self.col_no;

        // Try to parse using the parsing function provided
        let ret = parse_fn(self);

        if ret.is_err() {
            // Backtrack
            self.pos = pos;
            self.line_no = line_no;
            self.col_no = col_no;
        }

        ret
    }
}
