use crate::ast::*;
use crate::parser::{ParseError};

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
        self.body.check_types()?;
        Ok(())
    }
}

impl Stmt
{
    pub fn check_types(&mut self) -> Result<(), ParseError>
    {
        match self {
            Stmt::Expr(expr) => {
                expr.eval_type()?;
            }

            Stmt::Return => {
                todo!();
            }

            Stmt::ReturnExpr(expr) => {
                let expr_type = expr.eval_type()?;

                todo!();
            }

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



                // TODO: check that var type matches
                todo!();



            }

            Stmt::Block(stmts) => {
                for stmt in stmts {
                    stmt.check_types()?;
                }
            }
        }

        Ok(())
    }
}

impl Expr
{
    pub fn eval_type(&mut self) -> Result<Type, ParseError>
    {








        todo!();
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
        //unit.resolve_syms().unwrap();
    }

    fn parse_file(file_name: &str)
    {
        use crate::parser::{parse_file};

        dbg!(file_name);
        let mut unit = crate::parser::parse_file(file_name).unwrap();
        //unit.resolve_syms().unwrap();
    }




}
