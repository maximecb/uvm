use std::collections::HashMap;
use crate::ast::*;
use crate::parser::{ParseError};

/// Represent an environment with multiple levels of scoping
#[derive(Default)]
struct Env
{
    scopes: Vec<HashMap<String, Decl>>,
}

impl Env
{
    fn push_scope(&mut self)
    {
        self.scopes.push(HashMap::default());
    }

    fn pop_scope(&mut self)
    {
        self.scopes.pop();
    }

    fn define(&mut self, name: &str, decl: Decl)
    {
        let num_scopes = self.scopes.len();
        let top_scope = &mut self.scopes[num_scopes - 1];
        top_scope.insert(name.to_string(), decl);
    }

    fn lookup(&self, name: &str) -> Option<Decl>
    {
        let top_idx = self.scopes.len() - 1;

        for idx in top_idx..=0 {
            let scope = &self.scopes[idx];

            if let Some(decl) = scope.get(name) {
                return Some(decl.clone());
            }
        }

        return None;
    }
}

impl Unit
{
    pub fn resolve_syms(&mut self) -> Result<(), ParseError>
    {
        let mut env = Env::default();

        //
        // TODO: handle global variables
        //

        for fun in &mut self.fun_decls {
            fun.resolve_syms(&mut env)?;
        }

        Ok(())
    }
}

impl Function
{
    fn resolve_syms(&mut self, env: &mut Env) -> Result<(), ParseError>
    {
        env.push_scope();

        // Declare the function arguments
        for (idx, (param_type, param_name)) in self.params.iter().enumerate() {
            let decl = Decl::Arg { idx, t: param_type.clone() };
            env.define(param_name, decl);
        }

        self.body.resolve_syms(env)?;

        env.pop_scope();

        Ok(())
    }
}

impl Stmt
{
    fn resolve_syms(&mut self, env: &mut Env) -> Result<(), ParseError>
    {
        match self {
            Stmt::Expr(expr) => {
                expr.resolve_syms(env)?;
            }

            Stmt::Return => {}

            Stmt::ReturnExpr(expr) => {
                expr.resolve_syms(env)?;
            }

            /*
            If {
                test_expr: Expr,
                then_stmt: Box<Stmt>,
                else_stmt: Option<Box<Stmt>>,
            },

            While {
                test_expr: Expr,
                body_stmt: Box<Stmt>,
            },

            For {
                init_stmt: Option<Box<Stmt>>,
                test_expr: Expr,
                incr_expr: Expr,
                body_stmt: Box<Stmt>,
            },

            /// Local variable declaration
            VarDecl {
                var_type: Type,
                var_name: String,
                init_expr: Expr,
            }
            */

            Stmt::Block(stmts) => {
                env.push_scope();

                for stmt in stmts {
                    stmt.resolve_syms(env)?;
                }

                env.pop_scope();
            }

            _ => todo!()
        }

        Ok(())
    }
}

impl Expr
{
    fn resolve_syms(&mut self, env: &mut Env) -> Result<(), ParseError>
    {
        match self {
            Expr::Int(_) | Expr::String(_) => {}

            Expr::Ident(name) => {


            }

            /*
            Unary {
                op: UnOp,
                child: Box<Expr>,
            },

            Binary {
                op: BinOp,
                lhs: Box<Expr>,
                rhs: Box<Expr>,
            },

            Call {
                callee: Box<Expr>,
                args: Vec<Expr>,
            }
            */

            _ => todo!()
        }

        Ok(())
    }
}

#[cfg(test)]
mod tests
{
    use super::*;
    use crate::parser::{Input, parse_unit};

    fn parse_ok(src: &str)
    {
        dbg!(src);
        let mut input = Input::new(&src, "src");
        let mut unit = parse_unit(&mut input).unwrap();
        unit.resolve_syms().unwrap();
    }

    #[test]
    fn globals()
    {
    }

    #[test]
    fn basics()
    {
        parse_ok("void main() {}");
        parse_ok("void foo(u64 argc) {}");
    }







}
