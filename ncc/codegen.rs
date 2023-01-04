use crate::ast::*;
use crate::parser::{ParseError};
use crate::types::*;

#[derive(Default)]
struct SymGen
{
    next_id: usize,
}

impl SymGen
{
    fn gen_sym(&mut self, prefix: &str) -> String
    {
        let name = format!("_{}_{}", prefix, self.next_id);
        self.next_id += 1;
        name
    }
}

// FIXME: ideally, all error checking should be done before we get to the
// codegen, so that codegen can't return an error?

impl Unit
{
    pub fn gen_code(&self) -> Result<String, ParseError>
    {
        let mut sym = SymGen::default();
        let mut out: String = "".to_string();

        out.push_str(".data;\n");
        out.push_str("\n");

        //
        // TODO: globals
        //

        out.push_str(".code;\n");
        out.push_str("\n");

        for fun in &self.fun_decls {
            fun.gen_code(&mut sym, &mut out)?;
        }

        Ok((out))
    }
}

impl Function
{
    fn gen_code(&self, sym: &mut SymGen, out: &mut String) -> Result<(), ParseError>
    {
        // Emit label for function
        out.push_str(&format!("{}:\n", self.name));

        // Allocate stack slots for the local variables
        for i in 0..self.num_locals {
            out.push_str("push 0;\n");
        }

        self.body.gen_code(sym, out)?;

        out.push_str("\n");

        Ok(())
    }
}

impl Stmt
{
    fn gen_code(&self, sym: &mut SymGen, out: &mut String) -> Result<(), ParseError>
    {
        match self {
            Stmt::Expr(expr) => {
                expr.gen_code(out)?;
                out.push_str("pop;\n");
            }

            //Stmt::Break
            //Stmt::Continue => {}

            // Return void
            Stmt::Return => {
                out.push_str("push 0;\n");
                out.push_str("ret;\n");
            }

            Stmt::ReturnExpr(expr) => {
                expr.gen_code(out)?;
                out.push_str("ret;\n");
            }

            Stmt::If { test_expr, then_stmt, else_stmt } => {
                test_expr.gen_code(out)?;

                let false_label = sym.gen_sym("if_false");

                // If false, jump to else stmt
                out.push_str(&format!("jz {};\n", false_label));

                if else_stmt.is_some() {
                    let join_label = sym.gen_sym("if_join");

                    then_stmt.gen_code(sym, out)?;
                    out.push_str(&format!("jmp {};\n", join_label));

                    out.push_str(&format!("{}:\n", false_label));
                    else_stmt.as_ref().unwrap().gen_code(sym, out)?;
                    out.push_str(&format!("{}:\n", join_label));
                }
                else
                {
                    then_stmt.gen_code(sym, out)?;
                    out.push_str(&format!("{}:\n", false_label));
                }
            }

            /*
            Stmt::While { test_expr, body_stmt } => {
                test_expr.eval_type()?;
                body_stmt.check_types()?;
            }
            */

            Stmt::For { init_stmt, test_expr, incr_expr, body_stmt } => {
                if init_stmt.is_some() {
                    init_stmt.as_ref().unwrap().gen_code(sym, out)?;
                }

                let loop_label = sym.gen_sym("for_loop");
                let cont_label = sym.gen_sym("for_cont");
                let break_label = sym.gen_sym("for_break");

                out.push_str(&format!("{}:\n", loop_label));
                test_expr.gen_code(out)?;
                out.push_str(&format!("jz {};\n", break_label));

                body_stmt.gen_code(sym, out)?;

                out.push_str(&format!("{}:\n", cont_label));
                incr_expr.gen_code(out)?;
                out.push_str("pop;\n");
                out.push_str(&format!("jmp {};\n", loop_label));

                out.push_str(&format!("{}:\n", break_label));
            }

            /*
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
                    stmt.gen_code(sym, out)?;
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
                    Decl::Local { idx, .. } => {
                        out.push_str(&format!("get_local {};\n", idx));
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

                // Assignments are different from other kinds of expressions
                // because we don't evaluate the lhs the same way
                if *op == Assign {
                    gen_assign(lhs, rhs, out)?;
                    return Ok(());
                }

                lhs.as_ref().gen_code(out)?;
                rhs.as_ref().gen_code(out)?;

                match op {
                    // For now we're ignoring the type
                    Add => {
                        out.push_str("add_u64;\n");
                    }

                    Mul => {
                        out.push_str("mul_u64;\n");
                    }

                    /*
                    Eq | Ne | Lt | Gt => {
                        Ok(UInt(8))
                    }
                    */

                    Lt => {
                        out.push_str("lt_i64;\n");
                    }

                    _ => todo!("{:?}", op),
                }
            }

            //Expr::Call { callee, args } => todo!(),

            _ => todo!()
        }

        Ok(())
    }
}

fn gen_assign(lhs: &Expr, rhs: &Expr, out: &mut String) -> Result<(), ParseError>
{
    rhs.gen_code(out)?;

    // Assignment expressions must produce an output value
    out.push_str("dup;\n");

    match lhs {
        Expr::Ref(decl) => {
            match decl {
                Decl::Arg { idx, .. } => {
                    out.push_str(&format!("set_arg {};\n", idx));
                }
                Decl::Local { idx, .. } => {
                    out.push_str(&format!("set_local {};\n", idx));
                }
                _ => todo!()
            }
        }
        _ => todo!()
    }

    Ok(())
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
        parse_ok("u64 foo(u64 a) { return a + 1; }");
        parse_ok("u64 foo(u64 a) { return a; }");
        parse_ok("bool foo(u64 a, u64 b) { return a < b; }");

        // Local variables
        parse_ok("void main() { u64 a = 0; }");
        parse_ok("void main(u64 a) { u64 a = 0; }");
        parse_ok("void main(u64 a) { u64 b = a + 1; }");

        // Infix expressions
        parse_ok("u64 foo(u64 a, u64 b) { return a + b * 2; }");
    }

    #[test]
    fn if_else()
    {
        parse_ok("void foo(u64 a) { if (a) {} }");
        parse_ok("void foo(u64 a) { if (a) {} else {} }");
    }

    #[test]
    fn for_loop()
    {
        parse_ok("void foo(size_t n) { for (;;) {} }");
        parse_ok("void foo(size_t n) { for (size_t i = 0;;) {} }");
        parse_ok("void foo(size_t n) { for (size_t i = 0; i < n;) {} }");
        parse_ok("void foo(size_t n) { for (size_t i = 0; i < n; i = i + 1) {} }");
    }

    #[test]
    fn parse_files()
    {
        //parse_file("examples/fill_rect.c");
    }
}
