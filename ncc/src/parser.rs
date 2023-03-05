use std::collections::HashMap;
use std::io;
use std::io::Read;
use std::cmp::max;
use crate::parsing::*;
use crate::ast::*;

/// Parse an atomic expression
fn parse_atom(input: &mut Input) -> Result<Expr, ParseError>
{
    input.eat_ws()?;
    let ch = input.peek_ch();

    // Hexadecimal integer literal
    if input.match_token("0x")? {
        let val = input.parse_int(16)?;
        return Ok(Expr::Int(val));
    }

    // Binary integer literal
    if input.match_token("0b")? {
        let val = input.parse_int(2)?;
        return Ok(Expr::Int(val));
    }

    // Decimal integer literal
    if ch.is_digit(10) {
        let val = input.parse_int(10)?;
        return Ok(Expr::Int(val));
    }

    if input.match_keyword("NULL")? || input.match_keyword("null")? {
        return Ok(Expr::Int(0));
    }

    if input.match_keyword("true")? {
        return Ok(Expr::Int(1));
    }

    if input.match_keyword("false")? {
        return Ok(Expr::Int(0));
    }

    // String literal
    if ch == '\"' {
        let mut str_val = "".to_string();
        loop
        {
            str_val += &input.parse_str('"')?;
            input.eat_ws()?;
            if input.peek_ch() != '\"' {
                break;
            }
        }

        return Ok(Expr::String(str_val));
    }

    // Character literal
    if ch == '\'' {
        let char_str = input.parse_str('\'')?;
        let chars: Vec<char> = char_str.chars().collect();

        if chars.len() != 1 {
            return input.parse_error("invalid character constant");
        }

        return Ok(Expr::Int(chars[0] as i128));
    }

    // Parenthesized expression or type casting expression
    if ch == '(' {
        input.eat_ch();

        // Try to parse this as a type casting expression
        let new_type = input.with_backtracking(|input| parse_type(input));
        if let Ok(new_type) = new_type {
            input.expect_token(")")?;
            let child_expr = parse_prefix(input)?;

            return Ok(Expr::Cast {
                new_type,
                child: Box::new(child_expr)
            });
        }

        // Try parsing this as an expression
        let expr = parse_expr(input)?;
        input.expect_token(")")?;
        return Ok(expr);
    }

    // Array literal
    if ch == '{' {
        input.eat_ch();
        let elem_exprs = parse_expr_list(input, "}")?;
        return Ok(Expr::Array(elem_exprs));
    }

    // Sizeof expression
    if input.match_token("sizeof")? {
        input.expect_token("(")?;

        // Try to parse this as sizeof(type)
        let t = input.with_backtracking(|input| parse_type(input));
        if let Ok(t) = t {
            input.expect_token(")")?;
            return Ok(Expr::SizeofType { t });
        }

        // Try parsing this as sizeof(expr)
        let expr = parse_expr(input)?;
        input.expect_token(")")?;
        return Ok(Expr::SizeofExpr {
            child: Box::new(expr)
        });
    }

    // Inline assembly expression
    if input.match_token("asm")? {
        return parse_asm_expr(input);
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
    let arg_exprs = parse_expr_list(input, ")")?;

    Ok(Expr::Call {
        callee: Box::new(callee),
        args: arg_exprs
    })
}

/// Parse a postfix expression
fn parse_postfix(input: &mut Input) -> Result<Expr, ParseError>
{
    let mut base_expr = parse_atom(input)?;

    loop
    {
        // If this is a function call
        if input.match_token("(")? {
            base_expr = parse_call_expr(input, base_expr)?;
            continue;
        }

        // Array indexing
        if input.match_token("[")? {
            let index_expr = parse_expr(input)?;
            input.expect_token("]")?;

            // Transform into dereferencing and pointer addition
            base_expr = Expr::Unary {
                op: UnOp::Deref,
                child: Box::new(Expr::Binary {
                    op: BinOp::Add,
                    lhs: Box::new(base_expr),
                    rhs: Box::new(index_expr),
                })
            };

            continue;
        }

        // Postfix increment expression
        if input.match_token("++")? {
            // Let users know this is not supported. We use panic!() because
            // backtracking may override our error message.
            panic!(concat!(
                "the postfix increment operator (i.e. i++) is not supported, ",
                "use prefix increment (i.e. ++i) instead."
            ));
        }

        // Postfix decrement expression
        if input.match_token("--")? {
            // Let users know this is not supported. We use panic!() because
            // backtracking may override our error message.
            panic!(concat!(
                "the postfix increment operator (i.e. i--) is not supported, ",
                "use prefix increment (i.e. --i) instead."
            ));
        }

        break;
    }

    Ok(base_expr)
}

/// Parse an prefix expression
/// Note: this function should only call parse_postfix directly
/// to respect the priority of operations in C
fn parse_prefix(input: &mut Input) -> Result<Expr, ParseError>
{
    input.eat_ws()?;
    let ch = input.peek_ch();

    // Unary logical not expression
    if ch == '!' {
        input.eat_ch();
        let sub_expr = parse_prefix(input)?;

        return Ok(Expr::Unary{
            op: UnOp::Not,
            child: Box::new(sub_expr)
        });
    }

    // Pre-increment expression
    if input.match_token("++")? {
        let sub_expr = parse_prefix(input)?;

        // Transform into i = i + 1
        return Ok(
            Expr::Binary{
                op: BinOp::Assign,
                lhs: Box::new(sub_expr.clone()),
                rhs: Box::new(Expr::Binary{
                    op: BinOp::Add,
                    lhs: Box::new(sub_expr.clone()),
                    rhs: Box::new(Expr::Int(1))
                })
            }
        );
    }

    // Pre-decrement expression
    if input.match_token("--")? {
        let sub_expr = parse_prefix(input)?;

        // Transform into i = i - 1
        return Ok(
            Expr::Binary{
                op: BinOp::Assign,
                lhs: Box::new(sub_expr.clone()),
                rhs: Box::new(Expr::Binary{
                    op: BinOp::Sub,
                    lhs: Box::new(sub_expr.clone()),
                    rhs: Box::new(Expr::Int(1))
                })
            }
        );
    }

    // Unary negation expression
    if ch == '-' {
        input.eat_ch();
        let sub_expr = parse_prefix(input)?;

        // If this is an integer value, negate it
        if let Expr::Int(int_val) = sub_expr {
            return Ok(Expr::Int(-int_val));
        }

        return Ok(Expr::Unary{
            op: UnOp::Minus,
            child: Box::new(sub_expr)
        });
    }

    // Unary bitwise not expression
    if ch == '~' {
        input.eat_ch();
        let sub_expr = parse_prefix(input)?;

        return Ok(Expr::Unary{
            op: UnOp::BitNot,
            child: Box::new(sub_expr)
        });
    }

    // Pointer dereference
    if ch == '*' {
        input.eat_ch();
        let sub_expr = parse_prefix(input)?;

        return Ok(Expr::Unary{
            op: UnOp::Deref,
            child: Box::new(sub_expr)
        });
    }

    // Address of operator
    if ch == '&' {
        input.eat_ch();
        let sub_expr = parse_prefix(input)?;

        return Ok(Expr::Unary{
            op: UnOp::AddressOf,
            child: Box::new(sub_expr)
        });
    }

    // Try to parse this as a postfix expression
    parse_postfix(input)
}

/// Parse a list of argument expressions
fn parse_expr_list(input: &mut Input, end_token: &str) -> Result<Vec<Expr>, ParseError>
{
    let mut arg_exprs = Vec::default();

    loop {
        input.eat_ws()?;

        if input.eof() {
            return input.parse_error("unexpected end of input in call expression");
        }

        if input.match_token(end_token)? {
            break;
        }

        // Parse one argument
        arg_exprs.push(parse_infix_expr(input, true)?);

        if input.match_token(end_token)? {
            break;
        }

        // If this isn't the last argument, there
        // has to be a comma separator
        input.expect_token(",")?;
    }

    Ok(arg_exprs)
}

/// Parse an inline assembly expression
fn parse_asm_expr(input: &mut Input) -> Result<Expr, ParseError>
{
    input.expect_token("(")?;
    let arg_exprs = parse_expr_list(input, ")")?;
    input.expect_token("->")?;
    let out_type = parse_type(input)?;
    input.expect_token("{")?;

    let mut text = "".to_string();

    loop {
        if input.eof() {
            return input.parse_error("unexpected end of input in asm expression");
        }

        let ch = input.peek_ch();

        // If this is the end of the asm expression
        if ch == '}' {
            input.eat_ch();
            break;
        }

        input.eat_ch();
        text.push(ch);

        // Consume whitespace after newlines
        if ch == '\n' {
            loop {
                let ch = input.peek_ch();

                // Consume whitespace characters
                if !ch.is_ascii_whitespace()
                {
                    break;
                }

                input.eat_ch();
            }
        }
    }

    // Trim leading and trailing whitespace
    let text = text.trim().to_string();

    Ok(Expr::Asm {
        text,
        args: arg_exprs,
        out_type
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
const BIN_OPS: [OpInfo; 22] = [
    OpInfo { op_str: "->", prec: 1, op: BinOp::Arrow, rtl: false },
    OpInfo { op_str: ".", prec: 1, op: BinOp::Member, rtl: false },

    OpInfo { op_str: "*", prec: 3, op: BinOp::Mul, rtl: false },
    OpInfo { op_str: "/", prec: 3, op: BinOp::Div, rtl: false },
    OpInfo { op_str: "%", prec: 3, op: BinOp::Mod, rtl: false },
    OpInfo { op_str: "+", prec: 4, op: BinOp::Add, rtl: false },
    OpInfo { op_str: "-", prec: 4, op: BinOp::Sub, rtl: false },

    OpInfo { op_str: "<<", prec: 5, op: BinOp::LShift, rtl: false },
    OpInfo { op_str: ">>", prec: 5, op: BinOp::RShift, rtl: false },

    OpInfo { op_str: "<=", prec: 6, op: BinOp::Le, rtl: false },
    OpInfo { op_str: "<" , prec: 6, op: BinOp::Lt, rtl: false },
    OpInfo { op_str: ">=", prec: 6, op: BinOp::Ge, rtl: false },
    OpInfo { op_str: ">" , prec: 6, op: BinOp::Gt, rtl: false },
    OpInfo { op_str: "==", prec: 7, op: BinOp::Eq, rtl: false },
    OpInfo { op_str: "!=", prec: 7, op: BinOp::Ne, rtl: false },

    // Logical and, logical or
    // We place these first because they are longer tokens
    OpInfo { op_str: "&&", prec: 11, op: BinOp::And, rtl: false },
    OpInfo { op_str: "||", prec: 12, op: BinOp::Or, rtl: false },

    OpInfo { op_str: "&", prec: 8, op: BinOp::BitAnd, rtl: false },
    OpInfo { op_str: "^", prec: 9, op: BinOp::BitXor, rtl: false },
    OpInfo { op_str: "|", prec: 10, op: BinOp::BitOr, rtl: false },

    // Assignment operator, evaluates right to left
    OpInfo { op_str: "=", prec: 14, op: BinOp::Assign, rtl: true },

    // Sequencing operator
    OpInfo { op_str: ",", prec: 15, op: BinOp::Comma, rtl: false },
];

/// Precedence level of the ternary operator (a? b:c)
const TERNARY_PREC: usize = 13;

/// Try to match a binary operator in the input
fn match_bin_op(input: &mut Input, no_comma: bool) -> Result<Option<OpInfo>, ParseError>
{
    for op_info in BIN_OPS {
        if no_comma && op_info.op_str == "," {
            continue;
        }

        if input.match_token(op_info.op_str)? {
            return Ok(Some(op_info));
        }
    }

    Ok(None)
}

fn parse_expr(input: &mut Input) -> Result<Expr, ParseError>
{
    parse_infix_expr(input, false)
}

/// Parse a complex infix expression
/// This uses the shunting yard algorithm to parse infix expressions:
/// https://en.wikipedia.org/wiki/Shunting_yard_algorithm
fn parse_infix_expr(input: &mut Input, no_comma: bool) -> Result<Expr, ParseError>
{
    // Operator stack
    let mut op_stack: Vec<OpInfo> = Vec::default();

    // Expression stack
    let mut expr_stack: Vec<Expr> = Vec::default();

    // Parse the prefix sub-expression
    expr_stack.push(parse_prefix(input)?);

    // Evaluate the operators on the stack with lower
    // precedence than a new operator we just read
    fn eval_lower_prec(op_stack: &mut Vec<OpInfo>, expr_stack: &mut Vec<Expr>, new_op_prec: usize)
    {
        while op_stack.len() > 0 {
            // Get the operator at the top of the stack
            let top_op = &op_stack[op_stack.len() - 1];

            if top_op.prec <= new_op_prec {
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
    }

    loop
    {
        if input.eof() {
            break;
        }

        // Ternary operator
        if input.match_token("?")? {
            // We have to evaluate lower-precedence operators now
            // in order to use the resulting value for the boolean test
            eval_lower_prec(&mut op_stack, &mut expr_stack, TERNARY_PREC);

            let test_expr = expr_stack.pop().unwrap();
            let then_expr = parse_expr(input)?;
            input.expect_token(":")?;
            let else_expr = parse_expr(input)?;

            expr_stack.push(Expr::Ternary {
                test_expr: Box::new(test_expr),
                then_expr: Box::new(then_expr),
                else_expr: Box::new(else_expr),
            });

            break;
        }

        let new_op = match_bin_op(input, no_comma)?;

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

        // Evaluate the operators with lower precedence than
        // the new operator we just read
        eval_lower_prec(&mut op_stack, &mut expr_stack, new_op.prec);

        op_stack.push(new_op);

        // There must be another prefix sub-expression following
        expr_stack.push(parse_prefix(input)?);
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
        if input.eof() {
            return input.parse_error("unexpected end of input in block statement");
        }

        if input.match_token("}")? {
            break;
        }

        if input.match_token(";")? {
            // Empty statements are ignored
            continue;
        }

        stmts.push(parse_stmt(input)?);
    }

    return Ok(Stmt::Block(stmts));
}

/// Try to parse a variable declaration
fn parse_decl(input: &mut Input) -> Result<(Type, String, Option<Expr>), ParseError>
{
    let var_type = parse_type(input)?;
    let var_name = input.parse_ident()?;

    // For now, no support for local array variables
    // This would need alloca() to work
    //let var_type = parse_array_type(input, var_type)?;

    let init_expr = if input.match_token("=")? {
        Some(parse_expr(input)?)
    } else {
        None
    };

    Ok((var_type, var_name, init_expr))
}

/// Parse a statement
fn parse_stmt(input: &mut Input) -> Result<Stmt, ParseError>
{
    input.eat_ws()?;

    if input.match_keyword("return")? {
        if input.match_token(";")? {
            return Ok(Stmt::ReturnVoid);
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

    if input.match_keyword("break")? {
        input.expect_token(";")?;
        return Ok(Stmt::Break);
    }

    if input.match_keyword("continue")? {
        input.expect_token(";")?;
        return Ok(Stmt::Continue);
    }

    // If-else statement
    if input.match_keyword("if")? {
        // Parse the test expression
        input.expect_token("(")?;
        let test_expr = parse_expr(input)?;
        input.expect_token(")")?;

        // Parse the then statement
        let then_stmt = parse_stmt(input)?;

        // If there is an else statement
        if input.match_keyword("else")? {
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
    if input.match_keyword("while")? {
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

    // Do-while loop
    if input.match_keyword("do")? {

        // Parse the loop body
        let body_stmt = parse_stmt(input)?;

        // Parse the test expression
        input.expect_token("while")?;
        input.expect_token("(")?;
        let test_expr = parse_expr(input)?;
        input.expect_token(")")?;
        input.expect_token(";")?;

        return Ok(Stmt::DoWhile {
            test_expr,
            body_stmt: Box::new(body_stmt),
        });
    }

    // For loop
    if input.match_keyword("for")? {
        input.expect_token("(")?;

        let init_stmt = if input.match_token(";")? {
            None
        }
        else
        {
            Some(Box::new(parse_stmt(input)?))
        };

        let test_expr = if input.match_token(";")? {
            Expr::Int(1)
        }
        else
        {
            let test_expr = parse_expr(input)?;
            input.expect_token(";")?;
            test_expr
        };

        let incr_expr = if input.match_token(")")? {
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

    // Block statement
    if input.peek_ch() == '{' {
        return parse_block_stmt(input);
    }

    // Try to parse this as a variable declaration
    let var_decl = input.with_backtracking(|input| parse_decl(input));
    if let Ok((var_type, var_name, init_expr)) = var_decl {
        input.expect_token(";")?;

        return Ok(Stmt::VarDecl {
            var_type,
            var_name,
            init_expr,
        });
    }

    // Try to parse this as an expression statement
    let expr = parse_expr(input)?;
    input.expect_token(";")?;
    Ok(Stmt::Expr(expr))
}

/// Parse an atomic type expression
fn parse_type_atom(input: &mut Input) -> Result<Type, ParseError>
{
    input.eat_ws()?;
    let keyword = input.parse_ident()?;

    match keyword.as_str() {
        "void" => Ok(Type::Void),

        // Unsigned integer types
        "u8" => Ok(Type::UInt(8)),
        "u16" => Ok(Type::UInt(16)),
        "u32" => Ok(Type::UInt(32)),
        "u64" => Ok(Type::UInt(64)),

        // Signed integer types
        "i8" => Ok(Type::Int(8)),
        "i16" => Ok(Type::Int(16)),
        "i32" => Ok(Type::Int(32)),
        "i64" => Ok(Type::Int(64)),

        "size_t" => Ok(Type::UInt(64)),
        "char" => Ok(Type::UInt(8)),
        "bool" => Ok(Type::UInt(8)),

        // Standard integer types
        "short" => Ok(Type::Int(16)),
        "int" => Ok(Type::Int(32)),
        "long" => Ok(Type::Int(64)),

        // Unsigned qualifier
        "unsigned" => {
            if input.match_token("char")? {
                return Ok(Type::UInt(8));
            }

            let base_type = parse_type_atom(input)?;

            match base_type {
                Type::Int(n) => Ok(Type::UInt(n)),
                _ => input.parse_error("invalid type after unsigned qualifier")
            }
        }

        "float" => Ok(Type::Float(32)),

        _ => input.parse_error(&format!("unknown type {}", keyword))
    }

}

/// Parse a type name
fn parse_type(input: &mut Input) -> Result<Type, ParseError>
{
    let mut cur_type = parse_type_atom(input)?;

    loop
    {
        if input.match_token("*")? {
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
fn parse_array_type(input: &mut Input, base_type: Type) -> Result<Type, ParseError>
{
    if input.match_token("[")? {
        let size_expr = parse_atom(input)?;
        input.expect_token("]")?;

        let base_type = parse_array_type(input, base_type)?;

        Ok(Type::Array {
            elem_type: Box::new(base_type),
            size_expr: Box::new(size_expr),
        })
    }
    else
    {
        Ok(base_type)
    }
}

/// Parse a function declaration
fn parse_function(input: &mut Input, name: String, ret_type: Type, inline: bool) -> Result<Function, ParseError>
{
    let mut params = Vec::default();

    loop
    {
        input.eat_ws()?;

        if input.eof() {
            return input.parse_error("unexpected end of input inside function parameter list");
        }

        if input.match_token(")")? {
            break;
        }

        // Parse one parameter and its type
        let param_type = parse_type(input)?;
        let param_name = input.parse_ident()?;
        let param_type = parse_array_type(input, param_type)?;
        params.push((param_type, param_name));

        if input.match_token(")")? {
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
        inline,
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
        input.eat_ws()?;

        // If this is the end of the input
        if input.eof() {
            break;
        }

        // If this is an inline function attribute
        let inline = input.match_token("inline")?;

        // Parse the global declaration type and name
        let decl_type = parse_type(input)?;
        input.eat_ws()?;
        let name = input.parse_ident()?;

        // If this is the beginning of a function declaration
        if input.match_token("(")? {
            let fun = parse_function(input, name, decl_type, inline)?;
            unit.fun_decls.push(fun);
            continue;
        }

        // If we parsed a function attribute
        if inline {
            return input.parse_error("expected function declaration");
        }

        let decl_type = parse_array_type(input, decl_type)?;

        // Global variable initialization
        let init_expr = if input.match_token("=")? {
            Some(parse_expr(input)?)
        }
        else
        {
            None
        };

        // This must be a global variable declaration
        input.expect_token(";")?;

        unit.global_vars.push(Global {
            name,
            var_type: decl_type,
            init_expr
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
    let mut input = Input::from_file(file_name);
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
        parse_ok("/* Hi! */");
        parse_ok("/* Hi\nthere */");
        parse_ok("/* Hi\n/*there*/ */");

        parse_fails("x");
        parse_fails("x;");
        parse_fails("/* Hi\nthere");
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
        parse_ok("void foo() { /* hello! */}");
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
    fn empty_stmt()
    {
        parse_ok("void foo() { ; }");
        parse_ok("void foo() { {}; }");
        parse_ok("void foo() { if (1) {}; }");
    }

    #[test]
    fn globals()
    {
        parse_ok("size_t x;");
        parse_ok("size_t x = 20;");
        parse_ok("size_t x; void main() {}");
        parse_ok("size_t x; u64 y; void main() {}");
        parse_ok("u64 v = -1;");
        parse_ok("unsigned int v = 1;");
        parse_ok("unsigned long v = 1;");

        parse_ok("char* str = \"FOO\n\";");

        parse_ok("u8* pixel_buffer; u64 x; u64 y; void main() {}");
        parse_ok("u8 pixel_buffer[100]; void main() {}");
        parse_ok("u8 pixel_buffer[800][600]; void main() {}");
        parse_ok("u8 pixel_buffer[WIDTH][HEIGHT]; void main() {}");

        // Regression
        parse_ok("u8 g0;//\n//\n//\nu8 g1;");

        // Should fail
        parse_fails("u64x;");
    }

    #[test]
    fn arrays()
    {
        parse_ok("u8 array[3] = {};");
        parse_ok("u8 array[3] = { 0, 1, 2 };");
    }

    #[test]
    fn chars_strings()
    {
        parse_ok("void foo() { char c = 'f'; }");
        parse_ok("void foo() { char c = '\n'; }");
        parse_ok("void foo() { char* s = \"foo\"; }");
        parse_ok("void foo() { char* s = \"foo\" \"bar\"; }");
        parse_ok("void foo() { char* s = \"foo\"\n\"bar\"; }");
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
        parse_ok("u64 foo(u64* p) { return 1 + p[0] + 2; }");

        // Comma operator
        parse_ok("u64 foo() { return 1, 2; }");
        parse_ok("u64 foo() { return 1 + (1, 2); }");
        parse_ok("u64 foo() { return 1, 2, 3; }");
        parse_ok("u64 foo() { return 1, 2 + 3; }");
        parse_ok("u64 foo() { return 1 + 2, 3; }");

        // Should not parse
        parse_fails("u64 foo() { return 1 + 2 +; }");
    }

    #[test]
    fn cast_exprs()
    {
        parse_ok("int foo() { (int)1; }");
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
    fn asm_expr()
    {
        parse_ok("void main() { asm () -> void {}; }");
        parse_ok("void main() { asm (1, 2, 3) -> u64 {}; }");
        parse_ok("void main() { asm (1, 2, 3) -> u64 { push 1; }; }");
        parse_ok("void main() { asm (1, 2, 3) -> u64 { push 1;\n push2; }; }");
    }

    #[test]
    fn local_vars()
    {
        parse_ok("void main() { u64 x = 0; return; }");
        parse_ok("void main() { u32 crc = 0xFFFFFFFF; return; }");
        parse_ok("void main() { u64 x = 0; u64 y = x + 1; return; }");
        parse_ok("void main() { u64 x = 0; foo(x); return; }");
        parse_ok("u8 global; void main() { u8* p = &global; return; }");
        parse_ok("u8* global; void main() { u8 p = *global; return; }");

        // TODO:
        //parse_ok("void main() { u8 x[100] = 0; return; }");
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
    fn do_while_stmt()
    {
        parse_ok("void main() { do {} while (1); }");
        parse_fails("void main() { do {} while (1) }");
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
}
