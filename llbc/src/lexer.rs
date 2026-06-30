use rustc_hash::FxHashMap as HashMap;
use std::fs;
use std::fmt;
use std::sync::{Mutex, OnceLock};

#[derive(Default)]
struct FileIdMap
{
    // Map of file names to unique ids
    name_to_id: HashMap<String, u32>,

    // Map of integer ids to file names
    id_to_name: Vec<String>,
}

// Define the global hash map using OnceLock with u32 keys
static FILE_ID_MAP: OnceLock<Mutex<FileIdMap>> = OnceLock::new();

/// Helper function to get or initialize the global map
fn get_file_id_map() -> &'static Mutex<FileIdMap>
{
    FILE_ID_MAP.get_or_init(|| Mutex::new(FileIdMap::default()))
}

/// Get a unique id for a given file name
fn get_file_id(name: &str) -> u32
{
    let mut map = get_file_id_map().lock().unwrap();

    if let Some(id) = map.name_to_id.get(name) {
        return *id;
    }

    let new_id = map.id_to_name.len() as u32;
    map.id_to_name.push(name.to_owned());
    map.name_to_id.insert(name.to_owned(), new_id);
    new_id
}

/// Get the file name associated with a unique id
fn name_from_id(id: u32) -> String
{
    let id = id as usize;
    let map = get_file_id_map().lock().unwrap();
    assert!(id < map.id_to_name.len());
    map.id_to_name[id].clone()
}

#[derive(Debug, Copy, Clone, Default, Eq, PartialEq, Hash)]
pub struct SrcPos
{
    line_no: u32,
    col_no: u32,
    file_id: u32,
}

impl SrcPos
{
    pub fn get_src_name(&self) -> String
    {
        name_from_id(self.file_id)
    }
}

impl fmt::Display for SrcPos
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let src_name = name_from_id(self.file_id);
        write!(f, "{}@{}:{}", src_name, self.line_no, self.col_no)
    }
}

#[derive(Debug, Clone)]
pub struct ParseError
{
    pub msg: String,
    pub pos: SrcPos,
}

impl ParseError
{
    pub fn new(input: &Lexer, msg: &str) -> Self
    {
        ParseError {
            msg: msg.to_string(),
            pos: input.get_pos(),
        }
    }

    /// Parse error with just an error message and position
    pub fn with_pos<T>(msg: &str, pos: &SrcPos) -> Result<T, ParseError>
    {
        Err(ParseError {
            msg: msg.to_string(),
            pos: *pos,
        })
    }

    /// Parse error with just an error message, no location
    pub fn msg_only<T>(msg: &str) -> Result<T, ParseError>
    {
        Err(ParseError {
            msg: msg.to_string(),
            pos: SrcPos::default(),
        })
    }
}

impl fmt::Display for ParseError
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        if self.pos.line_no != 0 {
            write!(f, "{}: {}",  self.pos, self.msg)
        } else
        {
            write!(f, "{}", self.msg)
        }
    }
}

/// Check if a character can be the start of an identifier
pub fn is_ident_start(ch: char) -> bool
{
    ch.is_ascii_alphabetic() || ch == '_'
}

/// Check if a character can be part of an identifier
pub fn is_ident_ch(ch: char) -> bool
{
    ch.is_ascii_alphanumeric() || ch == '_'
}

#[derive(Debug, Clone)]
pub struct Lexer
{
    // Lexer string to be parsed
    input: Vec<char>,

    // Current index in the input string
    idx: usize,

    // Source file id
    pub file_id: u32,

    // Current line number
    pub line_no: u32,

    // Current column number
    pub col_no: u32,
}

impl Lexer
{
    pub fn from_file(file_name: &str) -> Result<Self, ParseError>
    {
        let data = match fs::read_to_string(file_name) {
            Ok(data) => data,
            Err(_) => {
                return Err(ParseError {
                    msg: format!("could not read input file \"{}\"", file_name),
                    pos: SrcPos::default()
                })
            }
        };

        Ok(Self::new(&data, file_name))
    }

    pub fn new(input_str: &str, src_name: &str) -> Self
    {
        let file_id = get_file_id(src_name);

        Self {
            input: input_str.chars().collect(),
            file_id,
            idx: 0,
            line_no: 1,
            col_no: 1
        }
    }

    pub fn get_src_name(&self) -> String
    {
        name_from_id(self.file_id)
    }

    pub fn get_pos(&self) -> SrcPos
    {
        SrcPos {
            line_no: self.line_no,
            col_no: self.col_no,
            file_id: self.file_id,
        }
    }

    pub fn set_pos(&mut self, pos: SrcPos)
    {
        assert!(pos.line_no > 0);
        assert!(pos.col_no > 0);
        self.line_no = pos.line_no;
        self.col_no = pos.col_no;
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

    /// Peek for a sequence of characters
    pub fn peek_chars(&mut self, chars: &[char]) -> bool
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

        return true;
    }

    /// Match characters in the input, no preceding whitespace allowed
    pub fn match_chars(&mut self, chars: &[char]) -> bool
    {
        if !self.peek_chars(chars) {
            return false;
        }

        // Consume the matched characters
        for _ in 0..chars.len() {
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

            // LLVM single-line comment, runs until the end of the line
            if self.peek_ch() == ';'
            {
                self.eat_comment();
                continue;
            }

            let ch = self.peek_ch();

            // Consume ASCII whitespace characters
            // Explicitly reject non-ASCII whitespace
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
        // Consume preceding whitespace
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
        fn read_digits(input: &mut Lexer)
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

        fn read_sign(input: &mut Lexer)
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
        // Eat the opening character
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

                    // Hexadecimal escape sequence
                    'x' => {
                        let digit0 = self.eat_ch().to_digit(16);
                        let digit1 = self.eat_ch().to_digit(16);

                        match (digit0, digit1) {
                            (Some(d0), Some(d1)) => {
                                let byte_val = ((d0 << 4) + d1) as u8;
                                out.push(byte_val as char);
                            }
                            _ => return self.parse_error("invalid hexadecimal escape sequence")
                        }
                    }

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

        if self.eof() || !is_ident_start(self.peek_ch()) {
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
}
