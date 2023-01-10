use crate::ast::*;
use crate::parser::{ParseError};
use std::cmp::{max};
use Type::*;

// TODO: we should probably automatically insert type promotions
// and type casting operations in assignments

/// Check if a value of one type can be assigned to another
fn assign_compat(lhs_type: &Type, rhs_type: &Type) -> bool
{
    match (&lhs_type, &rhs_type)
    {
        // If m < n, then the assignment truncates
        (UInt(m), UInt(n)) if m < n => true,

        // TODO: we need to enable sign-extension

        _ => lhs_type.eq(&rhs_type)
    }
}

impl Unit
{
    pub fn check_types(&mut self) -> Result<(), ParseError>
    {
        //
        // TODO: handle global variables
        //

        for fun in &mut self.fun_decls {
            fun.check_types()?;
        }

        Ok(())
    }
}

impl Function
{
    pub fn check_types(&mut self) -> Result<(), ParseError>
    {
        self.body.check_types(&self.ret_type)?;
        Ok(())
    }
}

impl Stmt
{
    pub fn check_types(&mut self, ret_type: &Type) -> Result<(), ParseError>
    {
        match self {
            Stmt::Expr(expr) => {
                expr.eval_type()?;
            }

            Stmt::Break | Stmt::Continue => {}

            // Return void
            Stmt::ReturnVoid => {
                if !ret_type.eq(&Type::Void) {
                    return ParseError::msg_only("return void in function not returning void");
                }
            }

            Stmt::ReturnExpr(expr) => {
                let expr_type = expr.eval_type()?;

                if !assign_compat(ret_type, &expr_type) {
                    return ParseError::msg_only("incompatible return type");
                }
            }

            Stmt::If { test_expr, then_stmt, else_stmt } => {
                test_expr.eval_type()?;
                then_stmt.check_types(ret_type)?;

                if else_stmt.is_some() {
                    else_stmt.as_mut().unwrap().check_types(ret_type)?;
                }
            }

            Stmt::While { test_expr, body_stmt } => {
                test_expr.eval_type()?;
                body_stmt.check_types(ret_type)?;
            }

            Stmt::For { init_stmt, test_expr, incr_expr, body_stmt } => {
                if init_stmt.is_some() {
                    init_stmt.as_mut().unwrap().check_types(ret_type)?;
                }

                test_expr.eval_type()?;
                incr_expr.eval_type()?;
                body_stmt.check_types(ret_type)?;
            }

            // Local variable declaration
            Stmt::VarDecl { var_type, var_name, init_expr } => {
                let expr_type = init_expr.eval_type()?;

                if !expr_type.eq(var_type) {
                    panic!();
                }
            }

            Stmt::Block(stmts) => {
                for stmt in stmts {
                    stmt.check_types(ret_type)?;
                }
            }
        }

        Ok(())
    }
}

impl Expr
{
    pub fn eval_type(&self) -> Result<Type, ParseError>
    {
        match self {
            Expr::Int(_) => {
                // TODO: we should probably get the smallest valid UInt type here
                Ok(UInt(64))
            }

            Expr::String(_) => {
                // TODO: this should be const char
                Ok(Pointer(Box::new(UInt(8))))
            }

            Expr::Ident(_) => panic!("IdentExpr made it past symbol resolution"),

            Expr::Ref(decl) => {
                Ok(decl.get_type())
            }

            Expr::Unary { op, child } => {
                let child_type = child.eval_type()?;

                match op {
                    UnOp::Minus => Ok(child_type),
                    UnOp::Not => Ok(child_type),

                    UnOp::Deref => {
                        match child_type {
                            Pointer(sub_type) => Ok(*sub_type.clone()),
                            _ => panic!()
                        }
                    }

                    _ => todo!()
                }
            },

            Expr::Binary { op, lhs, rhs } => {
                use BinOp::*;

                let lhs_type = lhs.eval_type()?;
                let rhs_type = rhs.eval_type()?;

                match op {
                    // TODO: we need to automatically insert type casting operations
                    // when the cast is valid
                    Assign => {
                        if !assign_compat(&lhs_type, &rhs_type) {
                            return ParseError::msg_only("rhs not assignable to lhs")
                        }

                        Ok(lhs_type)
                    }

                    Add | Sub => {
                        match (lhs_type, rhs_type) {
                            (UInt(m), UInt(n)) => Ok(UInt(max(m, n))),
                            (Pointer(b), UInt(n)) => Ok(Pointer(b)),
                            (UInt(n), Pointer(b)) => Ok(Pointer(b)),
                            _ => ParseError::msg_only("incompatible types in add/sub")
                        }
                    }

                    And | Or | Xor |
                    Mul | Div | Mod => {
                        match (lhs_type, rhs_type) {
                            (UInt(m), UInt(n)) => Ok(UInt(max(m, n))),
                            _ => ParseError::msg_only("incompatible types in arithmetic op")
                        }
                    }

                    Eq | Ne | Lt | Gt => {
                        Ok(UInt(8))
                    }

                    Comma => {
                        Ok(rhs_type)
                    }

                    //_ => todo!(),
                }
            }

            Expr::Call { callee, args } => {
                let fn_type = callee.eval_type()?;

                match fn_type {
                    Type::Fun { ret_type, param_types } => {
                        if args.len() != param_types.len() {
                            return ParseError::msg_only("argument count doesn't match function parameter count")
                        }

                        for (idx, arg) in args.iter().enumerate() {
                            let arg_type = arg.eval_type()?;

                            if !assign_compat(&param_types[idx], &arg_type) {
                                return ParseError::msg_only("argument type not compatible with parameter type")
                            }
                        }

                        Ok(*ret_type)
                    },
                    _ => ParseError::msg_only("callee is not a function")
                }
            }

            //_ => todo!()
        }
    }
}

#[cfg(test)]
mod tests
{
    use super::*;

    fn parse_ok(src: &str)
    {
        use crate::parser::{Input, parse_unit};

        dbg!(src);
        let mut input = Input::new(&src, "src");
        let mut unit = parse_unit(&mut input).unwrap();
        unit.resolve_syms().unwrap();
        unit.check_types().unwrap();
    }

    fn parse_file(file_name: &str)
    {
        use crate::parser::{parse_file};

        dbg!(file_name);
        let mut unit = crate::parser::parse_file(file_name).unwrap();
        unit.resolve_syms().unwrap();
        unit.check_types().unwrap();
    }

    #[test]
    fn calls()
    {
        parse_ok("void foo() {} void main() { foo(); }");
        parse_ok("u64 foo(u64 v) { return v; } void main() { foo(1); }");
        parse_ok("u64 foo(u64 a, u64 b) { return a + b; } void main() { foo(1, 2); }");

        // FIXME:
        //parse_ok("u64 foo(u64 v, u8* p) { return v; } void main() { foo(1, null); }");
    }

    #[test]
    fn parse_files()
    {
        parse_file("examples/fill_rect.c");
    }
}
