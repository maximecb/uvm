use crate::ast::*;
use crate::parser::{ParseError};
use crate::types::*;

// FIXME: ideally, all checking should be done before we get to the
// codegen, so that it can't return an error?

impl Unit
{
    fn gen_code(&self) -> Result<String, ParseError>
    {
        let mut out: String = "".to_string();

        //
        // TODO: globals
        //

        for fun in &self.fun_decls {
            fun.gen_code(&mut out)?;
        }

        Ok((out))
    }
}

impl Function
{
    fn gen_code(&self, out: &mut String) -> Result<(), ParseError>
    {
        // Emit label for function
        out.push_str(&format!("{}:\n", self.name));

        self.body.gen_code(out)?;

        out.push_str("\n");

        Ok(())
    }
}

impl Stmt
{
    fn gen_code(&self, out: &mut String) -> Result<(), ParseError>
    {
        match self {
            /*
            Stmt::Expr(expr) => {
                expr.eval_type()?;
            }

            Stmt::Break | Stmt::Continue => {}
            */

            // Return void
            Stmt::Return => {
                out.push_str("push 0;\n");
                out.push_str("ret;\n");
            }

            Stmt::ReturnExpr(expr) => {
                expr.gen_code(out)?;
                out.push_str("ret;\n");
            }

            /*
            Stmt::If { test_expr, then_stmt, else_stmt } => {
                test_expr.eval_type()?;
                then_stmt.check_types()?;

                if else_stmt.is_some() {
                    else_stmt.as_mut().unwrap().check_types()?;
                }
            }

            Stmt::While { test_expr, body_stmt } => {
                test_expr.eval_type()?;
                body_stmt.check_types()?;
            }

            Stmt::For { init_stmt, test_expr, incr_expr, body_stmt } => {
                if init_stmt.is_some() {
                    init_stmt.as_mut().unwrap().check_types()?;
                }

                test_expr.eval_type()?;
                incr_expr.eval_type()?;
                body_stmt.check_types()?;
            }

            // Local variable declaration
            Stmt::VarDecl { var_type, var_name, init_expr } => {
                let expr_type = init_expr.eval_type()?;

                if !expr_type.eq(var_type) {
                    panic!();
                }
            }
            */

            Stmt::Block(stmts) => {
                for stmt in stmts {
                    stmt.gen_code(out)?;
                }
            }

            _ => todo!()
        }

        Ok(())
    }
}

impl Expr
{
    fn gen_code(&self, out: &mut String) -> Result<(), ParseError>
    {
        match self {
            Expr::Int(v) => {
                out.push_str(&format!("push {};\n", v));
            }

            /*
            Expr::String(_) => {
                // TODO: this should be const char
                Ok(Pointer(Box::new(UInt(8))))
            }
            */

            Expr::Ref(decl) => {
                match decl {
                    Decl::Arg { idx, .. } => {
                        out.push_str(&format!("get_arg {};\n", idx));
                    }

                    _ => todo!()
                }
            }

            /*
            Expr::Unary { op, child } => {
                let child_type = child.as_mut().eval_type()?;

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
            */

            Expr::Binary { op, lhs, rhs } => {
                use BinOp::*;

                //let lhs_type = lhs.as_mut().eval_type()?;
                //let rhs_type = rhs.as_mut().eval_type()?;

                match op {
                    /*
                    Assign => {
                        if !lhs_type.eq(&rhs_type) {
                            return ParseError::msg_only("rhs not assignable to lhs")
                        }

                        Ok(lhs_type)
                    }
                    */

                    // For now we're ignoring the type
                    Add => {
                        out.push_str("add_u64\n;");
                    }

                    Mul => {
                        out.push_str("mul_u64\n;");
                    }

                    /*
                    Eq | Ne | Lt | Gt => {
                        Ok(UInt(8))
                    }
                    */

                    _ => todo!(),
                }
            }

            //Expr::Call { callee, args } => todo!(),

            _ => todo!()
        }

        Ok(())
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
        unit.gen_code().unwrap();
    }

    fn parse_file(file_name: &str)
    {
        use crate::parser::{parse_file};

        dbg!(file_name);
        let mut unit = crate::parser::parse_file(file_name).unwrap();
        unit.resolve_syms().unwrap();
        unit.check_types().unwrap();
        unit.gen_code().unwrap();
    }

    #[test]
    fn basics()
    {
        parse_ok("void main() {}");
        parse_ok("void foo(u64 a) {}");
        parse_ok("u64 foo(u64 a) { return a; }");

        // Local variables
        //parse_ok("void main() { u64 a = 0; }");
        //parse_ok("void main(u64 a) { u64 a = 0; }");

        // Infix expressions
        //parse_ok("u64 foo(u64 a, u64 b) { return a + b; }");
    }

    #[test]
    fn parse_files()
    {
        //parse_file("examples/fill_rect.c");
    }
}
