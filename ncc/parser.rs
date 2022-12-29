use std::collections::HashMap;
use std::fs;
use std::io;
use std::io::Read;
use std::fmt;
use std::cmp::max;
use crate::ast::*;

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
fn is_ident_ch(ch: char) -> bool
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

    /// Consume whitespace
    pub fn eat_ws(&mut self)
    {
        // Until the end of the whitespace
        loop
        {
            // If we are at the end of the input, stop
            if self.eof()
            {
                break;
            }

            // Single-line comments
            if self.match_chars(&['/', '/'])
            {
                loop
                {
                    // If we are at the end of the input, stop
                    if self.eof() || self.eat_ch() == '\n'
                    {
                        break;
                    }
                }
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

    /// Match a string in the input, ignoring preceding whitespace
    /// Do not use this method to match a keyword which could be
    /// an identifier.
    pub fn match_token(&mut self, token: &str) -> bool
    {
        // Consume preceding whitespace
        self.eat_ws();

        let token_chars: Vec<char> = token.chars().collect();
        return self.match_chars(&token_chars);
    }

    /// Match a keyword in the input, ignoring preceding whitespace
    /// This is different from match_token because there can't be a
    /// match if the following chars are also valid identifier chars.
    pub fn match_keyword(&mut self, keyword: &str) -> bool
    {
        self.eat_ws();

        let chars: Vec<char> = keyword.chars().collect();
        let end_pos = self.pos + chars.len();

        // We can't match as a keyword if the next chars are
        // valid identifier characters
        if end_pos < self.input_str.len() && is_ident_ch(self.input_str[end_pos]) {
            return false;
        }

        return self.match_chars(&chars);
    }

    /// Shortcut for yielding a parse error wrapped in a result type
    pub fn parse_error<T>(&self, msg: &str) -> Result<T, ParseError>
    {
        Err(ParseError::new(self, msg))
    }

    /// Produce an error if the input doesn't match a given token
    pub fn expect_token(&mut self, token: &str) -> Result<(), ParseError>
    {
        if self.match_token(token) {
            return Ok(())
        }

        self.parse_error(&format!("expected token \"{}\"", token))
    }

    /// Parse a decimal integer value
    pub fn parse_int(&mut self) -> Result<i128, ParseError>
    {
        let mut int_val: i128 = 0;

        if self.eof() || self.peek_ch().to_digit(10).is_none() {
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

            let digit = ch.to_digit(10);

            if digit.is_none() {
                break
            }

            int_val = 10 * int_val + digit.unwrap() as i128;
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

        if self.eof() || !self.peek_ch().is_ascii_alphabetic() {
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

/// Parse an atomic expression
fn parse_atom(input: &mut Input) -> Result<Expr, ParseError>
{
    input.eat_ws();
    let ch = input.peek_ch();

    // Decimal integer literal
    if ch.is_digit(10) {
        let val = input.parse_int()?;
        return Ok(Expr::Int(val));
    }

    // Unary negation expression
    if input.match_keyword("NULL") || input.match_keyword("null") {
        return Ok(Expr::Int(0));
    }

    // String literal
    if ch == '\"' {
        let str_val = input.parse_str()?;
        return Ok(Expr::String(str_val));
    }

    // Parenthesized expression
    if ch == '(' {
        input.eat_ch();
        let expr = parse_expr(input)?;
        input.expect_token(")")?;
        return Ok(expr);
    }

    // Unary logical not expression
    if ch == '!' {
        input.eat_ch();
        let sub_expr = parse_atom(input)?;

        return Ok(Expr::Unary{
            op: UnOp::Not,
            child: Box::new(sub_expr)
        });
    }

    // Unary negation expression
    if ch == '-' {
        input.eat_ch();
        let sub_expr = parse_atom(input)?;

        return Ok(Expr::Unary{
            op: UnOp::Minus,
            child: Box::new(sub_expr)
        });
    }

    // Pointer dereference
    if ch == '*' {
        input.eat_ch();
        let sub_expr = parse_atom(input)?;

        return Ok(Expr::Unary{
            op: UnOp::Deref,
            child: Box::new(sub_expr)
        });
    }

    // Address of operator
    if ch == '&' {
        input.eat_ch();
        let sub_expr = parse_atom(input)?;

        return Ok(Expr::Unary{
            op: UnOp::AddressOf,
            child: Box::new(sub_expr)
        });
    }

    // Identifier (variable reference)
    if is_ident_ch(ch) {
        let ident = input.parse_ident()?;

        return Ok(Expr::Ident(ident));
    }

    input.parse_error("unknown atomic expression")
}

/// Parse a function call expression
fn parse_call_expr(input: &mut Input, callee: Expr) -> Result<Expr, ParseError>
{
    let mut arg_exprs = Vec::default();

    loop {
        input.eat_ws();

        if input.eof() {
            return input.parse_error("unexpected end of input in call expression");
        }

        if input.match_token(")") {
            break;
        }

        // Parse one argument
        arg_exprs.push(parse_expr(input)?);

        if input.match_token(")") {
            break;
        }

        // If this isn't the last argument, there
        // has to be a comma separator
        input.expect_token(",")?;
    }

    Ok(Expr::Call {
        callee: Box::new(callee),
        args: arg_exprs
    })
}

struct OpInfo
{
    op_str: &'static str,
    prec: usize,
    op: BinOp,
    rtl: bool,
}

/// Binary operators and their precedence level
/// Lower numbers mean higher precedence
/// https://en.cppreference.com/w/c/language/operator_precedence
const BIN_OPS: [OpInfo; 9] = [
    OpInfo { op_str: "*", prec: 3, op: BinOp::Mul, rtl: false },
    OpInfo { op_str: "%", prec: 3, op: BinOp::Mod, rtl: false },
    OpInfo { op_str: "+", prec: 4, op: BinOp::Add, rtl: false },
    OpInfo { op_str: "-", prec: 4, op: BinOp::Sub, rtl: false },

    OpInfo { op_str: "<", prec: 6, op: BinOp::Lt, rtl: false },
    OpInfo { op_str: ">", prec: 6, op: BinOp::Gt, rtl: false },
    OpInfo { op_str: "==", prec: 7, op: BinOp::Eq, rtl: false },
    OpInfo { op_str: "!=", prec: 7, op: BinOp::Ne, rtl: false },

    // Assignment operator, evaluates right to left
    OpInfo { op_str: "=", prec: 14, op: BinOp::Ne, rtl: true },
];

/// Try to match a binary operator in the input
fn match_bin_op(input: &mut Input) -> Option<OpInfo>
{
    for op_info in BIN_OPS {
        if input.match_token(op_info.op_str) {
            return Some(op_info);
        }
    }

    None
}

/// Parse a complex expression
/// This uses the shunting yard algorithm to parse infix expressions:
/// https://en.wikipedia.org/wiki/Shunting_yard_algorithm
fn parse_expr(input: &mut Input) -> Result<Expr, ParseError>
{
    // Operator stack
    let mut op_stack: Vec<OpInfo> = Vec::default();

    // Expression stack
    let mut expr_stack: Vec<Expr> = Vec::default();

    // Parse the first atomic expression
    expr_stack.push(parse_atom(input)?);

    loop
    {
        if input.eof() {
            break;
        }

        // If this is a function call
        if input.match_token("(") {
            let callee = expr_stack.pop().unwrap();
            let call_expr = parse_call_expr(input, callee)?;
            expr_stack.push(call_expr);
            continue;
        }

        let new_op = match_bin_op(input);

        // If no operator could be matched, stop
        if new_op.is_none() {
            break;
        }

        let new_op = new_op.unwrap();

        // If this operator evaluates right-to-left,
        // e.g. an assignment operator
        if new_op.rtl == true {
            // Recursively parse the rhs expression,
            // forcing it to be evaluated before the lhs
            let rhs = parse_expr(input)?;

            let lhs = expr_stack.pop().unwrap();

            expr_stack.push(Expr::Binary {
                op: new_op.op,
                lhs: Box::new(lhs),
                rhs: Box::new(rhs)
            });

            break;
        }

        while op_stack.len() > 0 {
            // Get the operator at the top of the stack
            let top_op = &op_stack[op_stack.len() - 1];

            if top_op.prec < new_op.prec {
                assert!(expr_stack.len() >= 2);
                let rhs = expr_stack.pop().unwrap();
                let lhs = expr_stack.pop().unwrap();
                let top_op = op_stack.pop().unwrap();

                expr_stack.push(Expr::Binary {
                    op: top_op.op,
                    lhs: Box::new(lhs),
                    rhs: Box::new(rhs)
                });
            }
            else {
                break;
            }
        }

        op_stack.push(new_op);

        // There must be another expression following
        expr_stack.push(parse_atom(input)?);
    }

    // Emit all operators remaining on the operator stack
    while op_stack.len() > 0 {
        assert!(expr_stack.len() >= 2);
        let rhs = expr_stack.pop().unwrap();
        let lhs = expr_stack.pop().unwrap();
        let top_op = op_stack.pop().unwrap();

        expr_stack.push(Expr::Binary {
            op: top_op.op,
            lhs: Box::new(lhs),
            rhs: Box::new(rhs)
        });
    }

    assert!(expr_stack.len() == 1);
    Ok(expr_stack.pop().unwrap())
}

/// Parse a block statement
fn parse_block_stmt(input: &mut Input) -> Result<Stmt, ParseError>
{
    input.expect_token("{")?;

    let mut stmts = Vec::default();

    loop
    {
        input.eat_ws();

        if input.eof() {
            return input.parse_error("unexpected end of input in block statement");
        }

        if input.match_token("}") {
            break;
        }

        stmts.push(parse_stmt(input)?);
    }

    return Ok(Stmt::Block(stmts));
}

/// Try to parse a variable declaration
fn parse_decl(input: &mut Input) -> Result<(Type, String, Expr), ParseError>
{
    let var_type = parse_type(input)?;
    let var_name = input.parse_ident()?;

    // For now, no support for local array variables
    // This would need alloca() to work
    //let var_type = parse_array_type(input, var_type)?;

    input.expect_token("=")?;
    let init_expr = parse_expr(input)?;
    Ok((var_type, var_name, init_expr))
}

/// Parse a statement
fn parse_stmt(input: &mut Input) -> Result<Stmt, ParseError>
{
    input.eat_ws();

    if input.match_keyword("return") {
        if input.match_token(";") {
            return Ok(Stmt::Return);
        }
        else
        {
            let expr = parse_expr(input)?;
            input.expect_token(";")?;
            return Ok(
                Stmt::ReturnExpr(Box::new(expr))
            );
        }
    }

    if input.match_keyword("break") {
        input.expect_token("?")?;
        return Ok(Stmt::Break);
    }

    if input.match_keyword("continue") {
        input.expect_token("?")?;
        return Ok(Stmt::Continue);
    }

    // If-else statement
    if input.match_keyword("if") {
        // Parse the test expression
        input.expect_token("(")?;
        let test_expr = parse_expr(input)?;
        input.expect_token(")")?;

        // Parse the then statement
        let then_stmt = parse_stmt(input)?;

        // If there is an else statement
        if input.match_keyword("else") {
            // Parse the else statement
            let else_stmt = parse_stmt(input)?;

            return Ok(Stmt::If {
                test_expr,
                then_stmt: Box::new(then_stmt),
                else_stmt: Some(Box::new(else_stmt)),
            });
        }
        else
        {
            return Ok(Stmt::If {
                test_expr,
                then_stmt: Box::new(then_stmt),
                else_stmt: None
            });
        }
    }

    // While loop
    if input.match_keyword("while") {
        // Parse the test expression
        input.expect_token("(")?;
        let test_expr = parse_expr(input)?;
        input.expect_token(")")?;

        // Parse the loop body
        let body_stmt = parse_stmt(input)?;

        return Ok(Stmt::While {
            test_expr,
            body_stmt: Box::new(body_stmt),
        });
    }

    // For loop
    if input.match_keyword("for") {
        input.expect_token("(")?;

        let init_stmt = if input.match_token(";") {
            None
        }
        else
        {
            Some(Box::new(parse_stmt(input)?))
        };

        let test_expr = if input.match_token(";") {
            Expr::Int(1)
        }
        else
        {
            let test_expr = parse_expr(input)?;
            input.expect_token(";")?;
            test_expr
        };

        let incr_expr = if input.match_token(")") {
            Expr::Int(1)
        }
        else
        {
            let incr_expr = parse_expr(input)?;
            input.expect_token(")")?;
            incr_expr
        };

        // Parse the loop body
        let body_stmt = parse_stmt(input)?;

        return Ok(Stmt::For {
            init_stmt,
            test_expr,
            incr_expr,
            body_stmt: Box::new(body_stmt),
        });
    }

    /*
    // Assert statement
    if input.match_keyword("assert") {
        parse_expr(vm, input, fun, scope)?;
        input.expect_token(";")?;

        // If the expression is true, don't panic
        fun.insns.push(Insn::IfTrue { offset: 1 });
        fun.insns.push(Insn::Panic);

        return Ok(());
    }
    */

    // Block statement
    if input.peek_ch() == '{' {
        return parse_block_stmt(input);
    }

    // Save the current position for backtracking
    // TODO: make this into a convenient tuple?
    let pos = input.pos;
    let line_no = input.line_no;
    let col_no = input.col_no;

    // Try to parse this as a variable declaration
    if let Ok((var_type, var_name, init_expr)) = parse_decl(input) {
        input.expect_token(";")?;

        return Ok(Stmt::VarDecl {
            var_type,
            var_name,
            init_expr,
        });
    }

    // Backtrack
    input.pos = pos;
    input.line_no = line_no;
    input.col_no = col_no;

    // Try to parse this as an expression statement
    let expr = parse_expr(input)?;
    input.expect_token(";")?;
    Ok(Stmt::Expr(expr))
}

/// Parse an atomic type expression
fn parse_type_atom(input: &mut Input) -> Result<Type, ParseError>
{
    input.eat_ws();

    if input.match_keyword("void") {
        return Ok(Type::Void);
    }

    if input.match_keyword("u8") {
        return Ok(Type::UInt(8));
    }

    if input.match_keyword("char") {
        return Ok(Type::UInt(8));
    }

    if input.match_keyword("bool") {
        return Ok(Type::UInt(8));
    }

    if input.match_keyword("u64") {
        return Ok(Type::UInt(64));
    }

    if input.match_keyword("size_t") {
        return Ok(Type::UInt(64));
    }

    return input.parse_error("unknown type");
}

/// Parse a type name
fn parse_type(input: &mut Input) -> Result<Type, ParseError>
{
    input.eat_ws();

    let mut cur_type = parse_type_atom(input)?;

    loop
    {
        if input.match_token("*") {
            cur_type = Type::Pointer(
                Box::new(cur_type)
            );

            continue;
        }

        break;
    }

    Ok(cur_type)
}

/// Parse an array type
fn parse_array_type(input: &mut Input, elem_type: Type) -> Result<Type, ParseError>
{
    input.eat_ws();

    let mut cur_type = elem_type;

    loop
    {
        if input.match_token("[") {
            let size_expr = parse_atom(input)?;
            input.expect_token("]")?;

            cur_type = Type::Array {
                elem_type: Box::new(cur_type),
                size_expr: Box::new(size_expr),
            };

            continue;
        }

        break;
    }

    Ok(cur_type)
}

/// Parse a function declaration
fn parse_function(input: &mut Input, name: String, ret_type: Type) -> Result<Function, ParseError>
{
    let mut params = Vec::default();

    loop
    {
        input.eat_ws();

        if input.eof() {
            return input.parse_error("unexpected end of input inside function parameter list");
        }

        if input.match_token(")") {
            break;
        }

        // Parse one parameter and its type
        let param_type = parse_type(input)?;
        let param_name = input.parse_ident()?;
        let param_type = parse_array_type(input, param_type)?;
        params.push((param_type, param_name));

        if input.match_token(")") {
            break;
        }

        // If this isn't the last argument, there
        // has to be a comma separator
        input.expect_token(",")?;
    }

    // Parse the function body (must be a block statement)
    let body = parse_block_stmt(input)?;

    Ok(Function
    {
        name,
        ret_type,
        params,
        body,
        num_locals: 0,
    })
}

/// Parse a single unit of source code (e.g. one source file)
pub fn parse_unit(input: &mut Input) -> Result<Unit, ParseError>
{
    let mut unit = Unit::default();

    loop
    {
        input.eat_ws();

        if input.eof() {
            break;
        }

        let decl_type = parse_type(input)?;
        // TODO: parse_type().is_ok()

        input.eat_ws();
        let name = input.parse_ident()?;

        // If this is the beginning of a function declaration
        if input.match_token("(") {
            let fun = parse_function(input, name, decl_type)?;
            unit.fun_decls.push(fun);
            continue;
        }

        let decl_type = parse_array_type(input, decl_type)?;

        // This must be a global variable declaration
        input.expect_token(";")?;

        unit.global_vars.push(Global {
            name,
            var_type: decl_type,
        });
    }

    Ok(unit)
}

pub fn parse_str(src: &str) -> Result<Unit, ParseError>
{
    let mut input = Input::new(&src, "src");
    parse_unit(&mut input)
}

pub fn parse_file(file_name: &str) -> Result<Unit, ParseError>
{
    let data = fs::read_to_string(file_name)
        .expect(&format!("could not read input file {}", file_name));

    let mut input = Input::new(&data, file_name);

    parse_unit(&mut input)
}

#[cfg(test)]
mod tests
{
    use super::*;

    fn parse_ok(src: &str)
    {
        dbg!(src);
        let mut input = Input::new(&src, "src");
        parse_unit(&mut input).unwrap();
    }

    fn parse_fails(src: &str)
    {
        let mut input = Input::new(&src, "src");
        assert!(parse_unit(&mut input).is_err());
    }

    fn parse_file(file_name: &str)
    {
        dbg!(file_name);
        super::parse_file(file_name).unwrap();
    }

    #[test]
    fn simple_unit()
    {
        parse_ok("");
        parse_ok(" ");
        parse_ok("// Hi!\n ");
        parse_fails("x");
        parse_fails("x;");
    }

    #[test]
    fn fun_decl()
    {
        parse_ok("void main() {}");
        parse_ok("void main() { return; }");
        parse_ok("u64 main() { return 0; }");
        parse_ok("u64 main(u64 argc, char** argv) { return 0; }");
        parse_ok("void main(u64 argc, char** argv) {}");

        parse_ok("void foo() {}");
        //parse_ok("void foo() { /* hello! */}");
        parse_ok("u64 foo() {}");
        parse_ok("u64 foo() { {} }");
        parse_ok("u64 foo() { return (0); }");
        parse_ok("size_t foo() { return 0; }");
        parse_ok("u64 foo() { return -2; }");
        parse_ok("u64 foo() { return !1; }");
        parse_ok("u64 foo() { \"foo\"; return 77; }");
        parse_ok("u64 foo() { 333; return 77; }");
        parse_ok("char* foo() { return NULL; }");
        parse_ok("char** foo() { return NULL; }");
        parse_ok("u64 foo( u64 a , u64 b ) { return 77; }");

        // Should fail to parse
        parse_fails("u64 foo();");
        parse_fails("u64 foo() return 0;");
        parse_fails("void* f foo();");
        parse_fails("voidfoo() {}");
        parse_fails("void foo(u64 a, u64 b) { a = a b; }");
    }

    #[test]
    fn globals()
    {
        parse_ok("size_t x;");
        parse_ok("size_t x; void main() {}");
        parse_ok("size_t x; u64 y; void main() {}");

        parse_ok("u8* pixel_buffer; u64 x; u64 y; void main() {}");
        parse_ok("u8 pixel_buffer[100]; void main() {}");
        parse_ok("u8 pixel_buffer[800][600]; void main() {}");
        parse_ok("u8 pixel_buffer[WIDTH][HEIGHT]; void main() {}");

        // Should fail
        parse_fails("u64x;");
    }

    #[test]
    fn infix_exprs()
    {
        // Should parse
        parse_ok("u64 foo() { return 1 + 2; }");
        parse_ok("u64 foo() { return a + 1; }");
        parse_ok("u64 foo(u64 a, u64 b) { return a + b; }");
        parse_ok("u64 foo() { return 1 + 2 * 3; }");
        parse_ok("u64 foo() { return 1 + 2 + 3; }");
        parse_ok("u64 foo() { return 1 + 2 + 3 + 4; }");
        parse_ok("u64 foo() { return (1) + 2 + 3 * 4; }");

        // Should not parse
        parse_fails("u64 foo() { return 1 + 2 +; }");
    }

    #[test]
    fn call_expr()
    {
        parse_ok("void main() { foo(); }");
        parse_ok("void main() { foo(0); }");
        parse_ok("void main() { foo(0,); }");
        parse_ok("void main() { foo(0,1); }");
        parse_ok("void main() { foo( 0 , 1 , 2 , ); }");
        parse_ok("void main() { foo(0,1,2) + 3; }");
        parse_ok("void main() { foo(0,1,2) + bar(); }");
    }

    #[test]
    fn local_vars()
    {
        parse_ok("void main() { u64 x = 0; return; }");
        //parse_ok("void main() { u8 x[100] = 0; return; }");
        parse_ok("void main() { u64 x = 0; u64 y = x + 1; return; }");
        parse_ok("void main() { u64 x = 0; foo(x); return; }");
        parse_ok("u8 global; void main() { u8* p = &global; return; }");
        parse_ok("u8* global; void main() { u8 p = *global; return; }");
    }

    #[test]
    fn assign_stmt()
    {
        parse_ok("void main() { u64 x = 0; x = x + 1; return; }");
        parse_ok("void main() { u64 x = 0; u64 y = 0; x = y = 1; return; }");
        parse_ok("char* global; void main() { *global = 0; return; }");
    }

    #[test]
    fn while_stmt()
    {
        parse_ok("void main() { while (1) { foo(); } }");
        parse_ok("void foo(u64 n) { u64 i = 0; while (i < n) { foo(); i = i + 1; } }");
    }

    #[test]
    fn for_stmt()
    {
        parse_ok("void main() { for (;;) {} }");
        parse_ok("void main() { for (size_t i = 0;;) {} }");
        parse_ok("void main() { for (size_t i = 0; i < 10;) {} }");
        parse_ok("void main() { for (size_t i = 0; i < 10; i = i + 1) {} }");
    }

    #[test]
    fn if_stmt()
    {
        parse_ok("void main() { if (1) { foo(); } }");
        parse_ok("void main() { if (1) { foo(); } else { bar(); } }");
    }

    #[test]
    fn parse_files()
    {
        parse_file("examples/fill_rect.c");
    }
}
