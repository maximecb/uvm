use std::collections::HashMap;
use crate::ast::*;
use crate::parsing::{ParseError};

impl Unit
{
    pub fn insert_casts(&mut self) -> Result<(), ParseError>
    {
        for fun in &mut self.fun_decls {
            fun.insert_casts()?;
        }

        Ok(())
    }
}

impl Function
{
    fn insert_casts(&mut self) -> Result<(), ParseError>
    {
        self.body.insert_casts()?;

        Ok(())
    }
}

impl Stmt
{
    fn insert_casts(&mut self) -> Result<(), ParseError>
    {
        match self {
            Stmt::Expr(expr) => {
                expr.insert_casts()?;
            }

            Stmt::Break | Stmt::Continue => {}

            Stmt::ReturnVoid => {}

            Stmt::ReturnExpr(expr) => {
                expr.insert_casts()?;
            }

            Stmt::If { test_expr, then_stmt, else_stmt } => {
                test_expr.insert_casts()?;
                then_stmt.insert_casts()?;

                if else_stmt.is_some() {
                    else_stmt.as_mut().unwrap().insert_casts()?;
                }
            }

            Stmt::While { test_expr, body_stmt } => {
                test_expr.insert_casts()?;
                body_stmt.insert_casts()?;
            }

            Stmt::DoWhile { test_expr, body_stmt } => {
                test_expr.insert_casts()?;
                body_stmt.insert_casts()?;
            }

            Stmt::For { init_stmt, test_expr, incr_expr, body_stmt } => {
                if init_stmt.is_some() {
                    init_stmt.as_mut().unwrap().insert_casts()?;
                }

                test_expr.insert_casts()?;
                incr_expr.insert_casts()?;
                body_stmt.insert_casts()?;
            }

            // Local variable declaration
            Stmt::VarDecl { var_type, var_name, init_expr } => {
                // If there is an initiaization expression
                if let Some(init_expr) = init_expr {
                    init_expr.insert_casts()?;
                }
            }

            Stmt::Block(stmts) => {
                for stmt in stmts {
                    stmt.insert_casts()?;
                }
            }
        }

        Ok(())
    }
}

impl Expr
{
    fn insert_casts(&mut self) -> Result<(), ParseError>
    {
        match self {
            Expr::Int(_) => {}
            Expr::Float32(_) => {}

            Expr::String(str_const) => {}

            Expr::Array(exprs) => {
                for expr in exprs {
                    expr.insert_casts()?;
                }
            }

            Expr::Ident(name) => panic!(),

            Expr::Ref(_) => {},

            Expr::Cast { new_type, child } => {
                child.as_mut().insert_casts()?;
            }

            Expr::SizeofExpr { child } => {
                child.as_mut().insert_casts()?;
            }

            Expr::SizeofType { t } => {}

            Expr::Arrow { base, field } => {
                base.as_mut().insert_casts()?;
            }

            Expr::Unary { op, child } => {
                child.as_mut().insert_casts()?;
            }

            Expr::Binary { op, lhs, rhs } => {
                lhs.as_mut().insert_casts()?;
                rhs.as_mut().insert_casts()?;
            }

            Expr::Ternary { test_expr, then_expr, else_expr } => {
                test_expr.as_mut().insert_casts()?;
                then_expr.as_mut().insert_casts()?;
                else_expr.as_mut().insert_casts()?;
            }

            Expr::Call { callee, args } => {
                callee.insert_casts()?;
                for arg in args {
                    arg.insert_casts()?;
                }
            }

            Expr::Asm { args, out_type, .. } => {
                for arg in args {
                    arg.insert_casts()?;
                }
            }

            //_ => todo!()
        }

        Ok(())
    }
}
