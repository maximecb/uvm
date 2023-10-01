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
        /*
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
                env.push_scope();

                if init_stmt.is_some() {
                    init_stmt.as_mut().unwrap().insert_casts()?;
                }

                test_expr.insert_casts()?;
                incr_expr.insert_casts()?;

                body_stmt.insert_casts()?;

                env.pop_scope();
            }

            // Local variable declaration
            Stmt::VarDecl { var_type, var_name, init_expr } => {
                resolve_types(var_type, env, None)?;

                env.define_local(var_name, var_type.clone());

                let decl = env.lookup(var_name).unwrap();
                let ref_expr = Expr::Ref(decl);

                // If there is an initiaization expression
                if let Some(init_expr) = init_expr {
                    init_expr.insert_casts()?;

                    let assign_expr = Expr::Binary {
                        op: BinOp::Assign,
                        lhs: Box::new(ref_expr),
                        rhs: Box::new(init_expr.clone()),
                    };

                    *self = Stmt::Expr(assign_expr);
                }
                else
                {
                    *self = Stmt::Expr(Expr::Int(0));
                }
            }

            Stmt::Block(stmts) => {
                env.push_scope();

                for stmt in stmts {
                    stmt.insert_casts()?;
                }

                env.pop_scope();
            }
        }
        */

        Ok(())
    }
}

impl Expr
{
    fn insert_casts(&mut self) -> Result<(), ParseError>
    {
        /*
        match self {
            Expr::Int(_) => {}
            Expr::Float32(_) => {}

            Expr::String(str_const) => {
                // Get a global symbol for the string constant
                let decl = env.get_string(str_const);
                *self = Expr::Ref(decl);
            }

            Expr::Array(exprs) => {
                for expr in exprs {
                    expr.insert_casts()?;
                }
            }

            Expr::Ident(name) => {
                //dbg!(&name);

                if let Some(decl) = env.lookup(name) {
                    *self = Expr::Ref(decl);
                }
                else
                {
                    return ParseError::msg_only(&format!("reference to undeclared identifier \"{}\"", name));
                }
            }

            Expr::Ref(_) => panic!(),

            Expr::Cast { new_type, child } => {
                if let Type::Named(name) = new_type {
                    if let Some(Decl::TypeDef { name, t }) = env.lookup(name) {
                        *new_type = (**t).borrow().clone();
                    }
                    else
                    {
                        return ParseError::msg_only(&format!("reference to unknown type \"{}\" in cast expression", name));
                    }
                }
                else
                {
                    resolve_types(new_type, env, None)?;
                }

                child.as_mut().insert_casts()?;
            }

            Expr::SizeofExpr { child } => {
                child.as_mut().insert_casts()?;
            }

            Expr::SizeofType { t } => {
                if let Type::Named(name) = t {
                    if let Some(Decl::TypeDef { name, t: dt }) = env.lookup(name) {
                        *t = (**dt).borrow().clone();
                    }
                    else
                    {
                        *self = Expr::SizeofExpr {
                            child: Box::new(Expr::Ident(name.clone()))
                        };

                        self.insert_casts()?;
                    }
                }
                else
                {
                    resolve_types(t, env, None)?;
                }
            }

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

                resolve_types(out_type, env, None)?;
            }

            //_ => todo!()
        }
        */

        Ok(())
    }
}
