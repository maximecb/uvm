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
    col_no : u32,
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

    /*
    // String literal
    if ch == '\"' || ch == '\'' {
        let str_val = input.parse_str()?;
        let gc_val = vm.into_gc_heap(str_val);
        fun.insns.push(Insn::Push { val: gc_val });
        return Ok(());
    }

    // Parenthesized expression
    if ch == '(' {
        input.eat_ch();
        parse_expr(vm, input, fun, scope)?;
        input.expect_token(")")?;
        return Ok(());
    }
    */

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

    /*
    // Function expression
    if input.match_keyword("fun") {
        let mut new_fun = Function::new(&input.src_name);
        let mut scope = Scope::new(&mut new_fun);

        input.expect_token("(")?;

        loop {
            if input.eof() {
                return input.parse_error("end of file in function parameter list");
            }

            if input.match_token(")") {
                break;
            }

            let param_name = input.parse_ident()?;
            scope.decl_var(&param_name);
            new_fun.params.push(param_name);

            if input.match_token(")") {
                break;
            }

            input.expect_token(",")?;
        }

        // Parse the function body
        parse_stmt(vm, input, &mut new_fun, &mut scope)?;

        // TODO: need to GC allocate fun
        // TODO: need to push stmt on stack, Insn::Push
        fun.insns.push(Insn::Push{ val: Value::Nil });

        return Ok(());
    }

    // Identifier (variable reference)
    if is_ident_ch(ch) {
        let ident = input.parse_ident()?;

        // Check if there is a runtime function with this name
        let runtime_fn = get_runtime_fn(&ident);

        if runtime_fn.is_some() {
            let host_fn = Value::HostFn(runtime_fn.unwrap());
            fun.insns.push(Insn::Push { val: host_fn });
            return Ok(());
        }

        let local_idx = scope.lookup(&ident);

        // If the variable is not found
        if local_idx.is_none() {
            return input.parse_error(&format!("undeclared variable {}", ident));
        }

        // If this is actually an assignment
        if input.match_token("=") {
            // Parse the expression to assign
            parse_expr(vm, input, fun, scope)?;

            fun.insns.push(Insn::Dup);
            fun.insns.push(Insn::SetLocal{ idx: local_idx.unwrap() });
        }
        else
        {
            fun.insns.push(Insn::GetLocal{ idx: local_idx.unwrap() });
        }

        return Ok(());
    }
    */

    input.parse_error("unknown atomic expression")
}




/*
/// Parse a function call expression
fn parse_call_expr(vm: &mut VM, input: &mut Input, fun: &mut Function, scope: &mut Scope) -> Result<(), ParseError>
{
    // Note that the callee expression has already been parsed
    // when parse_call_expr is called

    let mut argc = 0;

    loop {
        input.eat_ws();

        if input.eof() {
            return input.parse_error("unexpected end of input in call expression");
        }

        if input.match_token(")") {
            break;
        }

        // Parse one argument
        parse_expr(vm, input, fun, scope)?;

        // Increment the argument count
        argc += 1;

        if input.match_token(")") {
            break;
        }

        // If this isn't the last argument, there
        // has to be a comma separator
        input.expect_token(",")?;
    }

    fun.insns.push(Insn::Call { argc });

    Ok(())
}
*/




struct OpInfo
{
    op: &'static str,
    prec: usize,
}

/// Binary operators and their precedence level
/// https://en.cppreference.com/w/c/language/operator_precedence
const BIN_OPS: [OpInfo; 8] = [
    OpInfo { op: "*", prec: 2 },
    OpInfo { op: "%", prec: 2 },
    OpInfo { op: "+", prec: 1 },
    OpInfo { op: "-", prec: 1 },
    OpInfo { op: "==", prec: 0 },
    OpInfo { op: "!=", prec: 0 },
    OpInfo { op: "<", prec: 0 },
    OpInfo { op: ">", prec: 0 },
];

/// Try to match a binary operator in the input
fn match_bin_op(input: &mut Input) -> Option<OpInfo>
{
    for op_info in BIN_OPS {
        if input.match_token(op_info.op) {
            return Some(op_info);
        }
    }

    None
}

/*
fn emit_op(op: &str, fun: &mut Function)
{
    match op {
        "*" => fun.insns.push(Insn::Mul),
        "%" => fun.insns.push(Insn::Mod),
        "+" => fun.insns.push(Insn::Add),
        "-" => fun.insns.push(Insn::Sub),
        "==" => fun.insns.push(Insn::Eq),
        "!=" => fun.insns.push(Insn::Ne),
        "<" => fun.insns.push(Insn::Lt),
        ">" => fun.insns.push(Insn::Gt),
        _ => panic!()
    }
}
*/





/// Parse a complex expression
/// This uses the shunting yard algorithm to parse infix expressions:
/// https://en.wikipedia.org/wiki/Shunting_yard_algorithm
fn parse_expr(input: &mut Input) -> Result<Expr, ParseError>
{
    // Operator stack
    let mut op_stack: Vec<OpInfo> = Vec::default();



    todo!();



    // Parse the first atomic expression
    //parse_atom(vm, input, fun, scope)?;

    /*
    loop
    {
        if input.eof() {
            break;
        }

        // If this is a function call
        if input.match_token("(") {
            parse_call_expr(vm, input, fun, scope)?;
            continue;
        }

        let new_op = match_bin_op(input);

        // If no operator could be matched, stop
        if new_op.is_none() {
            break
        }

        let new_op = new_op.unwrap();

        while op_stack.len() > 0 {
            // Get the operator at the top of the stack
            let top_op = &op_stack[op_stack.len() - 1];

            if top_op.prec > new_op.prec {
                emit_op(top_op.op, fun);
                op_stack.pop();
            }
            else {
                break;
            }
        }

        op_stack.push(new_op);

        // There must be another expression following
        parse_atom(vm, input, fun, scope)?;
    }

    // Emit all operators remaining on the operator stack
    while op_stack.len() > 0 {
        let top_op = &op_stack[op_stack.len() - 1];
        emit_op(top_op.op, fun);
        op_stack.pop();
    }

    Ok(())
    */
}

/// Parse a statement
fn parse_stmt(input: &mut Input) -> Result<Stmt, ParseError>
{
    input.eat_ws();

    if input.match_keyword("return") {
        let expr = parse_expr(input)?;
        input.expect_token(";")?;
        return Ok(Stmt::Return(Box::new(expr)));
    }

    todo!();

    /*
    // Variable declaration
    if input.match_keyword("let") {
        input.eat_ws();
        let ident = input.parse_ident()?;
        input.expect_token("=")?;
        parse_expr(vm, input, fun, scope)?;
        input.expect_token(";")?;

        // Check if there is a runtime function with this name
        let runtime_fn = get_runtime_fn(&ident);

        if runtime_fn.is_some() {
            let host_fn = Value::HostFn(runtime_fn.unwrap());
            fun.insns.push(Insn::Push { val: host_fn });
            return input.parse_error(&format!("there is already a runtime function named {}", ident));
        }

        if let Some(local_idx) = scope.decl_var(&ident) {
            fun.insns.push(Insn::SetLocal{ idx: local_idx });
            return Ok(());
        }
        else
        {
            return input.parse_error(&format!("variable {} already declared", ident));
        }
    }

    // If-else statement
    if input.match_keyword("if") {
        // Parse the test expression
        input.expect_token("(")?;
        parse_expr(vm, input, fun, scope)?;
        input.expect_token(")")?;

        // If the test evaluates to false, jump past the true statement
        let if_idx = fun.insns.len() as isize;
        fun.insns.push(Insn::IfFalse { offset: 0 });

        // Parse the true statement
        parse_stmt(vm, input, fun, scope)?;

        // If there is an else statement
        if input.match_keyword("else") {
            // After the true statement is done, jump over the else
            let true_jmp_idx = fun.insns.len() as isize;
            fun.insns.push(Insn::Jump { offset: 0 });

            // If the test evaluates to false, jump to the else statement
            let false_jmp_idx = fun.insns.len() as isize;
            let if_offset = false_jmp_idx - (if_idx + 1);
            fun.insns[if_idx as usize] = Insn::IfFalse { offset: if_offset };

            // Parse the false statement
            let false_stmt_idx = fun.insns.len();
            parse_stmt(vm, input, fun, scope)?;

            // Patch the true jump
            let end_idx = fun.insns.len() as isize;
            let true_jmp_offset = end_idx - (true_jmp_idx + 1);
            fun.insns[true_jmp_idx as usize] = Insn::Jump { offset: true_jmp_offset };
        }
        else
        {
            // If the test evaluates to false, jump after the true statement
            let false_jmp_idx = fun.insns.len() as isize;
            let if_offset = false_jmp_idx - (if_idx + 1);
            fun.insns[if_idx as usize] = Insn::IfFalse { offset: if_offset };
        }

        return Ok(());
    }

    // While loop
    if input.match_keyword("while") {
        // Parse the test expression
        input.expect_token("(")?;
        let test_idx = fun.insns.len() as isize;
        parse_expr(vm, input, fun, scope)?;
        input.expect_token(")")?;

        // If the test evaluates to false, jump past the loop body
        let if_idx = fun.insns.len() as isize;
        fun.insns.push(Insn::IfFalse { offset: 0 });

        // Parse the loop body
        parse_stmt(vm, input, fun, scope)?;

        // Jump back to the loop test
        let jump_idx = fun.insns.len() as isize;
        fun.insns.push(Insn::Jump { offset: test_idx - (jump_idx + 1) });

        // Patch the loop test jump offset
        fun.insns[if_idx as usize] = Insn::IfFalse { offset: (jump_idx + 1) - (if_idx + 1) };

        return Ok(());
    }

    // Assert statement
    if input.match_keyword("assert") {
        parse_expr(vm, input, fun, scope)?;
        input.expect_token(";")?;

        // If the expression is true, don't panic
        fun.insns.push(Insn::IfTrue { offset: 1 });
        fun.insns.push(Insn::Panic);

        return Ok(());
    }

    // Block statement
    if input.match_token("{") {
        // Create a nested scope for the block
        let mut scope = Scope::new_nested(scope);

        loop
        {
            input.eat_ws();

            if input.eof() {
                return input.parse_error("unexpected end of input in block statement");
            }

            if input.match_token("}") {
                break;
            }

            parse_stmt(vm, input, fun, &mut scope)?;
        }

        return Ok(());
    }

    // Try to parse this as an expression statement
    parse_expr(vm, input, fun, scope)?;
    fun.insns.push(Insn::Pop);
    input.expect_token(";")
    */
}

/// Parse a type name
pub fn parse_type(input: &mut Input) -> Result<Type, ParseError>
{
    input.eat_ws();

    if input.match_keyword("void") {
        return Ok(Type::Void);
    }

    if input.match_keyword("u64") {
        return Ok(Type::UInt64);
    }

    return input.parse_error("unknown type");
}

/// Parse a function declaration
pub fn parse_function(input: &mut Input, name: String, ret_type: Type) -> Result<Function, ParseError>
{


    todo!();






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
        }
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

    /*
    fn parse_ok(src: &str)
    {
        let mut vm = VM::new();
        let mut input = Input::new(&src, "src");
        assert!(parse_unit(&mut vm, &mut input).is_ok());
    }

    fn parse_fails(src: &str)
    {
        let mut vm = VM::new();
        let mut input = Input::new(&src, "src");
        assert!(parse_unit(&mut vm, &mut input).is_err());
    }

    #[test]
    fn int_token_int()
    {
        let mut input = Input::new("1 + 2", "input");
        input.eat_ws();
        assert_eq!(input.parse_int().unwrap(), 1);
        assert!(input.match_token("+"));
        input.eat_ws();
        assert_eq!(input.parse_int().unwrap(), 2);
        assert!(input.eof());
    }

    #[test]
    fn simple_str()
    {
        let mut input = Input::new(" \"foobar\"", "input");
        input.eat_ws();
        assert!(input.peek_ch() == '\"');
        assert_eq!(input.parse_str().unwrap(), "foobar");
        input.eat_ws();
        assert!(input.eof());
    }

    #[test]
    fn single_line_comment()
    {
        let mut input = Input::new("1 // test\n  2", "input");
        assert_eq!(input.parse_int().unwrap(), 1);
        input.eat_ws();
        assert_eq!(input.parse_int().unwrap(), 2);
        assert!(input.eof());
    }

    #[test]
    fn simple_unit()
    {
        parse_ok("");
        parse_ok(" ");
        parse_ok("1;");
        parse_ok("1; ");
        parse_ok(" \"foobar\";");
        parse_ok("'foo\tbar\nbif';");
        parse_ok("1_000_000;");
    }

    #[test]
    fn infix_exprs()
    {
        // Should parse
        parse_ok("1 + 2;");
        parse_ok("1 + 2 * 3;");
        parse_ok("1 + 2 + 3;");
        parse_ok("1 + 2 + 3 + 4;");
        parse_ok("(1) + 2 + 3 * 4;");

        // Should not parse
        parse_fails("1 + 2 +;");
    }

    #[test]
    fn stmts()
    {
        parse_ok("let x = 3;");
        parse_ok("let str = 'foo';");
        parse_ok("let x = 3; let y = 5;");
        parse_ok("{ let x = 3; x; } let y = 4;");

        parse_ok("assert 1;");
        parse_ok("let x = 3;");
        parse_ok("let x = 3; return x;");
        parse_fails("letx=3;");
        parse_fails("let x = 3; returnx;");
        parse_fails("assert1;");

        parse_ok("let x = 3; if (!x) x = 1;");
    }

    #[test]
    fn call_expr()
    {
        parse_ok("1();");
        parse_ok("1(0);");
        parse_ok("1(0,);");
        parse_ok("1(0,1);");
        parse_ok("1( 0 , 1 , 2 );");
        parse_ok("0 + 1(0,1,2) + 3;");
        parse_ok("let x = 1(0,1,2);");
    }

    #[test]
    fn runtime_fn()
    {
        parse_fails("let println = 3;");
        parse_fails("println = 3;");

        parse_ok("println(1);");
        parse_ok("println(1, 2);");
    }

    #[test]
    fn fun_expr()
    {
        parse_ok("let f = fun() {};");
        parse_ok("let f = fun(x) {};");
        parse_ok("let f = fun(x,) {};");
        parse_ok("let f = fun(x,y) {};");
        parse_ok("let f = fun(x,y) { return 1; };");
        parse_fails("let f = fun(x,y,1) {};");
    }
    */
}